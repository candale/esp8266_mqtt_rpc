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


class MQTTRouter:

    wildcard_regex_plus = b'([^/]+)'
    _RegexTopic = namedtuple('RegexTopic', ['ure_obj', 'group_count'])

    def __init__(self):
        self._handlers_map = []

    def _resolve_handlers(self, recv_topic):

        def get_args(match, group_count):
            return [match.group(index) for index in range(1, group_count + 1)]

        handlers = []

        for topic_cmp, handler in self._handlers_map:
            if isinstance(topic_cmp, self._RegexTopic):
                match = topic_cmp.ure_obj.match(recv_topic)
                if match:
                    # TODO: would be nice to send integers as integers not strings
                    args = get_args(match, topic_cmp.group_count)
                    handlers.append((handler, args))
            elif topic_cmp == recv_topic:
                handlers.append((handler, []))

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
        wildcard_count = topic.count(b'+')
        if wildcard_count:
            return self._RegexTopic(
                ure_obj=ure.compile(
                    topic.replace(b'+', self.wildcard_regex_plus)),
                group_count=wildcard_count)
        else:
            return topic

    def register_handler(self, topic, handler):
        '''
        Registers a handler
        '''
        # TODO: replace the regex if possible. i think it takes a lot of mem
        self._handlers_map.append((self._get_regex_if_needed(topic), handler))


class MQTTRpc:

    router_class = MQTTRouter
    # An iterable of the form ((<topic, <topic_handler_clas), ...)
    handler_classes = None
    unique_id = str(int.from_bytes(machine.unique_id()))
    spec_topic = 'devices/%s/spec' % unique_id

    def __init__(self, id, server):
        self._client = MQTTClient(id, server)
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

    def start(self, period=500):
        # TODO: call things in right order
        if self.router_class is None:
            raise ValueError('Improperly configured: no router configured')

        self._client.connect()

        if self._router is None:
            handlers_info = self._init_router()
            self._client.set_callback(self._router)

            for topic, spec in handlers_info:
                self._client.subscribe(topic)
                self._client.publish(
                    self.spec_topic,
                    '{}|{}'.format(topic.decode('utf-8'), spec))

        self._timer.init(
            period=period, mode=machine.Timer.PERIODIC,
            callback=lambda t: self._client.check_msg())

    def stop(self):
        self._client.disconnect()
        self._timer.deinit()
