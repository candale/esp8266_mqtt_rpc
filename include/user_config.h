#ifndef __USER_CONFIG_H_
#define __USER_CONFIG_H_

#define DEBUG_ON
// #define MQTT_DEBUG_ON

// WIFI CONFIGURATION
#define WIFI_SSID "ssid"
#define WIFI_PASS "pass"


// MQTT CONFIGURATION
#define MQTT_SSL_ENABLE
#define PROTOCOL_NAMEv31

#define MQTT_HOST       "212.47.229.77"
#define MQTT_PORT       1883
#define MQTT_BUF_SIZE   1024
#define MQTT_KEEPALIVE  120

#define MQTT_CLIENT_ID      "mycoolid"
#define MQTT_USER           "lol"
#define MQTT_PASS           "pass"
#define MQTT_CLEAN_SESSION  1
#define MQTT_KEEPALIVE      120

#define MQTT_RECONNECT_TIMEOUT  5 /*second*/

#define DEFAULT_SECURITY  0
#define QUEUE_BUFFER_SIZE 2048

// DEBUG OPTIONS
#if defined(DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#endif
