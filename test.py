from core import MessageHandlerBase, MQTTRpc


class ConsoleHandler(MessageHandlerBase):

    def process_message(self, msg=None):
        print(msg)

    def get_spec(self):
        return 'call|no_argument_handler|test handler|'


class OneArgumentHandler(MessageHandlerBase):

    def process_message(self, topic_kind, msg=None):
        print(topic_kind, msg)

    def get_spec(self):
        return 'call|one_argument_handler|test handler|string:topic_kind'


class TwoArgumentHandler(MessageHandlerBase):

    def process_message(self, topic_kind, topic_kind_level, msg=None):
        print(topic_kind, topic_kind_level, msg)

    def get_spec(self):
        return (
            'call|two_argument_handler|test handler|'
            'string:topic_kind,string:topic_kind_level')


class MyMQTTRpc(MQTTRpc):

    handler_classes = (
        (b'my/topic', ConsoleHandler),
        (b'my/+/topic', OneArgumentHandler),
        (b'my/+/+/topic', TwoArgumentHandler))


rpc = MyMQTTRpc(b'lol', '212.47.229.77')
rpc.start()
