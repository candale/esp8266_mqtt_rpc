from core import MessageHandlerBase, MQTTRpc


class ConsoleHandler(MessageHandlerBase):

    def process_message(self, msg=None):
        print(msg)


class MyMQTTRpc(MQTTRpc):

    handler_classes = ((b'my/topic', ConsoleHandler),)


rpc = MyMQTTRpc(b'lol', '212.47.229.77')
rpc.start()
