# ESP8266 Micropython MQTTRpc

Classes: `MessageHandlerBase`, `MQTTRouter` and `MQTTRpc`.


## Message handlers (operations)

On the device, one may define message handler (operation) classes that must implement a `process_message` and a `get_spec` method (the same as `MessageHandlerBase`) . 

The `process_message` method of a message handler is called whenever a message is received on a topic on which the operation class was registered.
`process_message` takes a keyword argument `msg` and zero or more positional arguments, depending on the format of the topic. 
For example, for a topic of the form `/this/is/a/+/topic` the method should look like this: `def process_message(self, what_kind_of_topic, msg=None)`.
Currently, only `+` wildcard is supported.

`get_spec` is called whenever the RPC starts running. The method must return a string that represents the specification of the operation in question. 
The format is flexible, but the specification must contain this information:

+ `op_type`: operation type; `call` for operations that are called to execute some code and `recv` for operations that publish data
+ `topic`: the topic on which the operation can be called (if `op_type` is `call`) or on which data is published (if `opt_type` is `recv`)
+ `op_name`: the name of the operation
+ `op_description`: description of the operation
+ `args`: a list of key-value arguments with the key being the type of the argument and the value, the name of the argument; currently the supported types are `str`, `int` and `bool`

## MQTTRouter

`MQTTRouter` is a class can take up pairs of topic and handler instance and executes `process_message` of one of the operations, given a topic. The class implements the `__call__` method that takes as positional parameters a topic and a message. It looks in its registry of topic-handler pairs and tries to match the received topic to one of them. If more than zero were found, it calls the found operations passing message and the arguments found in the topic.


## MQTTRpc

`MQTTRpc` is the class that brings it all together. The user is responsible with inheriting and configuring the this class.

### Attributes:

- `router_class`: the router class; default `MQTTRouter`
- `mqtt_client_class`: the class to be used as MQTT client; default `umqtt.simple.MQTTClient`
- `handler_class`: an iterable with topic-handler class tuples
- `name`: the unique name of the client; if name is not specified, the device id will be `machine.unique_id()`
- `last_will_retain`: retain flag for last will for the MQTT client; default `False`
- `last_will_qos`: quality of service for last will; default 0
- `server`: MQTT server host
- `port`: QMTT server port; default 1883
- `keepalive`: MQTT keepalive; default 180 seconds
- `self_keepalive`: whether or not to let MQTTRpc instance ping the MQTT server in order to keep the connection alive; the default behavior is to ping the server on interval of `keepalive` / 2


### Methods:

+ `get_id()` - Get the unique id of the device.
+ `start()` - Start processing messages
+ `stop()` - Disconnect from server.
