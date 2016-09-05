import ure
import machine
from ucollections import namedtuple

from umqtt.simple import MQTTClient


class MessageHandlerBase:

    def __init__(self, client):
        self._client = client

    def process_message(self, *args, msg=None):
        '''
        This method is called when a message is received on a topic that this
        handler is in charge of
        '''

    def get_spec(self):
        '''
        Gets the specification of this handler, a string of the from
        <func_type (callable, data_source)>|<function_name>|<description>|<arg_type: arg_name, ...>
        '''


def match_topic(raw_topic, to_match):
    '''
    Matches a topic to a raw topic
    Returns a a boolean that indicates if the topics match and a list
    with all the arguments that were send (instead of +)
    '''
    args = []
    if '+' not in raw_topic:
        return str(raw_topic) == str(to_match), args

    raw_topic_split = str(raw_topic).split('/')
    to_match_split = str(to_match).split('/')

    if len(raw_topic_split) != len(to_match_split):
        return False, args

    for raw, match in zip(raw_topic_split, to_match_split):
        if raw == '+':
            args.append(match)
        elif raw != match:
            return False, []

    return True, args


class MQTTRouter:

    def __init__(self):
        self._handlers_map = []

    def _resolve_handlers(self, recv_topic):
        handlers = []

        for topic_cmp, handler in self._handlers_map:
            is_match, args = match_topic(topic_cmp, recv_topic)
            if is_match:
                handlers.append((handler, args))

        return handlers

    def __call__(self, topic, msg):
        '''
        This is the method that receives all messages and sends everything
        to the appropriate registered callbacks
        '''
        handlers = self._resolve_handlers(topic)
        for handler, args in handlers:
            handler.process_message(*args, msg=msg)

    def _get_regex_if_needed(self, topic):
        wildcard_count = topic.count('+')
        if wildcard_count:
            return self._RegexTopic(
                ure_obj=ure.compile(
                    topic.replace('+', self.wildcard_regex_plus)),
                group_count=wildcard_count)
        else:
            return topic

    def register_handler(self, topic, handler):
        '''
        Registers a handler
        '''
        self._handlers_map.append((topic, handler))


class MQTTRpc:

    # TODO: add keepalive timer
    router_class = MQTTRouter
    # An iterable of the form ((<topic>, <topic_handler_class>), ...)
    handler_classes = None
    name = None
    last_will_retain = False
    last_will_qos = 0
    server = None
    port = 1883
    keepalive = 180

    _unique_id = str(int.from_bytes(machine.unique_id()))
    _spec_topic = 'devices/{}/spec'

    def __init__(self):
        assert self.server, 'No server'
        self._client = MQTTClient(
            self.get_id(), self.server, keepalive=self.keepalive)
        self._client.set_last_will(
            self.get_last_will_topic(), self.get_last_will_message(),
            qos=self.last_will_qos, retain=self.last_will_retain)
        self._router = None
        self._timer = machine.Timer(-1)

    def _init_router(self):
        if self.handler_classes is None:
            raise ValueError('Improperly configured: no handlers')

        self._router = self.router_class()
        handlers_info = []
        for topic, handler_cls in self.handler_classes:
            # TODO: maybe pass a wrapper of the client with limited functions
            #       (e.g. subscribe and send)
            handler = handler_cls(self._client)
            spec = handler.get_spec()

            if not spec:
                raise ValueError(
                    'Improperly configured: handler %s does not have spec'
                    % handler_cls.__name__)

            self._router.register_handler(topic, handler_cls(self._client))
            handlers_info.append((topic, spec))

        return handlers_info

    def get_id(self):
        '''
        The return value of this method is used as the id of the device
        '''
        if self.name is not None:
            return str(self.name) + '_' + self._unique_id
        else:
            return self._unique_id

    def get_last_will_topic(self):
        return 'devices/' + self.get_id() + '/offline'

    def get_last_will_message(self):
        return 'offline'

    def start(self, period=500):
        if self.router_class is None:
            raise ValueError('Improperly configured: no router configured')

        self._client.connect()

        if self._router is None:
            handlers_info = self._init_router()
            self._client.set_callback(self._router)

            for topic, spec in handlers_info:
                self._client.subscribe(topic)
                self._client.publish(
                    self._spec_topic.format(self.get_id()),
                    '{}|{}'.format(topic.decode('utf-8'), spec))

        # In case of failure, kill the timer somehow
        self._timer.init(
            period=period, mode=machine.Timer.PERIODIC,
            callback=lambda t: self._client.check_msg())

    def stop(self):
        self._client.disconnect()
        self._timer.deinit()
