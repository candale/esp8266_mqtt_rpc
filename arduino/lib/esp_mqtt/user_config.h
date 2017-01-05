#ifdef __cplusplus
extern "C" {
#endif

#ifndef __USER_CONFIG_H_
#define __USER_CONFIG_H_

#define DEBUG_ON

// MQTT CONFIGURATION
#define MQTT_SSL_ENABLE
#define PROTOCOL_NAMEv31
#define MQTT_BUF_SIZE   1024
#define MQTT_RECONNECT_TIMEOUT  5 /*second*/
#define DEFAULT_SECURITY  0
#define QUEUE_BUFFER_SIZE 2048

#endif

#ifdef __cplusplus
}
#endif
