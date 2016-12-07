#ifndef __MQTT_RPC_H_
#define __MQTT_RPC_H_

#include "mqtt.h"
#include "mem.h"
#include "osapi.h"


#define MQTT_RPC_DEBUG_ON

#define BASE_TOPIC "devices"
#define STATUS_TOPIC "status"
#define SPEC_TOPIC "spec"
#define ACK_TOPIC "ack"

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

#define MQTTRPC_INIT_CONF(...) { \
    .handlers = 0, \
    .offline_message = "-", \
    .online_message = "+", \
    .last_will_retain = 0, \
    .last_will_qos = 0, \
    .online_qos = 0, \
    .online_retain = 0, \
    .break_on_first = 0, \
    ## __VA_ARGS__ }

typedef struct _mqttrpc_conf MQTTRPC_Conf;
typedef struct _mqttrpc_topic_map MQTTRPC_Topic_Map;
typedef int (*MQTTRPC_Handler)(MQTTRPC_Conf* mqtt_rpc, char* data, char* args[], uint8_t arg_count);

struct _mqttrpc_conf {
    MQTT_Client* _mqtt_client;
    const MQTTRPC_Topic_Map* handlers;
    const char* offline_message;
    const char* online_message;
    const char* base_topic;
    const char* status_topic;
    const char* spec_topic;
    const char* ack_topic;
    const uint8_t online_qos;
    const uint8_t online_retain;
    const uint8_t last_will_retain;
    const uint8_t last_will_qos;
    const uint8_t break_on_first;
    void* user_data;
};

struct _mqttrpc_topic_map {
    MQTTRPC_Handler handler;
    char* topic;
    uint8_t qos;
};


void ICACHE_FLASH_ATTR
MQTTRPC_Init(MQTTRPC_Conf* rpc_conf, MQTT_Client* mqtt_client);

uint8_t ICACHE_FLASH_ATTR
MQTTRPC_Publish(
    MQTTRPC_Conf* config, const char* topic, const char* data,
    int data_len, int qos, int retain);


#if defined(MQTT_RPC_DEBUG_ON)
#define RPC_INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define RPC_INFO( format, ... )
#endif

#endif
