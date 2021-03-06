import ure
import ubinascii
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


# TODO: break this class somehow that it could run in CPython with little
#       changes ... maybe ... or just develop another, separate package
class MQTTRpc:

    router_class = MQTTRouter
    mqtt_client_class = MQTTClient
    # An iterable of the form ((<topic>, <topic_handler_class>), ...)
    handler_classes = None
    name = None
    last_will_retain = False
    last_will_qos = 0
    server = None
    port = 1883
    keepalive = 180
    self_keepalive = False

    username = None
    password = None

    # TODO: maybe rethink how this information is get..maybe have methods or
    #       have both methods and attributes, like queryset in drf
    offline_message = '-'
    online_message = '+'
    _unique_id = str(int.from_bytes(ubinascii.hexlify(machine.unique_id())))

    def __init__(self):
        self._router = None

        self.base_topic = (
            self.get_base_topic() or
            'device/' + (self.get_id() or self.name or self._unique_id) + '/')
        self.spec_topic = self.base_topic + 'spec'
        self.status_topic = self.base_topic + 'status'
        self.ack_topic = self.base_topic + 'ack'

        self._check_msg_timer = machine.Timer(-1)
        self._keepalive_timer = machine.Timer(-1)

        self._make_client()

    def _make_client(self):
        assert self.server, 'No server'
        self._client = self.mqtt_client_class(
            self.get_id() or self.name or self._unique_id, self.server,
            keepalive=self.keepalive, user=self.username,
            password=self.password)
        self._client.set_last_will(
            self.status_topic, self.offline_message,
            qos=self.last_will_qos, retain=self.last_will_retain)

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

            # TODO: refactor this as it is used in multiple places
            prefixed_topic = self.base_topic.encode() + topic

            self._router.register_handler(prefixed_topic, handler_cls(self._client))
            handlers_info.append((prefixed_topic, spec))

        return handlers_info

    def _safe_mqtt_call(self, callback):
        '''
        Robust version for calling mqtt methods. In case of connection error,
        it will try to reconnect.
        Inspired by umqtt.robust
        '''
        try:
            callback()
            return
        except OSError:
            for i in range(2):
                try:
                    self._connect(False)
                    print('mqtt connection back')
                    callback()
                    return
                except OSError:
                    pass
        print('mqtt lost connection')

    def _connect(self, new_session=True):
        self._client.connect(new_session)
        self._client.publish(self.status_topic, self.online_message)

    def _disconnect(self):
        self._client.publish(self.status_topic, self.offline_message)
        self._client.disconnect()

    def _on_msg(self, topic, msg):
        self._router(topic, msg)
        self._client.publish(self.ack_topic, '+')

    def start(self, period=500):
        if self.router_class is None:
            raise ValueError('Improperly configured: no router configured')

        self._connect()

        if self._router is None:
            handlers_info = self._init_router()
            self._client.set_callback(self._on_msg)

            for topic, spec in handlers_info:
                self._client.subscribe(topic)
                self._client.publish(
                    self.spec_topic,
                    '{}|{}'.format(topic.decode('utf-8'), spec))

        self._check_msg_timer.init(
            period=period, mode=machine.Timer.PERIODIC,
            callback=lambda t: self._safe_mqtt_call(self._client.check_msg))

        if self.keepalive and self.self_keepalive:
            self._keepalive_timer.init(
                period=int(self.keepalive / 2) * 1000,
                mode=machine.Timer.PERIODIC,
                callback=lambda t: self._safe_mqtt_call(self._client.ping))

    def stop(self):
        self._check_msg_timer.deinit()
        self._keepalive_timer.deinit()
        self._disconnect()

    def get_id(self):
        '''
        The return value of this method is used as the id of the device
        '''

    def get_base_topic(self):
        '''
        The return value of this method will be used as a prefix for all
        topics in the application
        '''
