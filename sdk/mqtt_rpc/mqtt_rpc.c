#include "mqtt_rpc.h"


static void ICACHE_FLASH_ATTR
mqtt_connected_cb(uint32_t *args)
{
    RPC_INFO("MQTTRPC: MQTT client connected");

    MQTT_Client* mqtt_client = (MQTT_Client*)args;
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)mqtt_client->user_data;
    const MQTTRPC_Topic_Map* handlers = rpc_conf->handlers;

    // Subscribe for all handlers
    while(handlers->topic != 0) {
        MQTT_Subscribe(mqtt_client, handlers->topic, handlers->qos);
        handlers++;
    }
}

static void ICACHE_FLASH_ATTR
mqtt_disconnected_cb(uint32_t *args)
{
    RPC_INFO("MQTTRPC: MQTT client disconnected");
    MQTT_Client* client = (MQTT_Client*)args;
}

static void ICACHE_FLASH_ATTR
mqtt_published_cb(uint32_t *args)
{
    RPC_INFO("MQTTRPC: MQTT message published");

    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}

static void ICACHE_FLASH_ATTR
mqtt_data_cb(uint32_t *args, const char* topic, uint32_t topic_len,
             const char *data, uint32_t data_len)
{
    RPC_INFO("MQTTRPC: MQTT client got data");
    char *topicBuf = (char*)os_zalloc(topic_len + 1),
        *dataBuf = (char*)os_zalloc(data_len + 1);

    MQTT_Client* client = (MQTT_Client*)args;
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)client->user_data;
    const MQTTRPC_Topic_Map* topic_map = rpc_conf->handlers;

    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);

    while(topic_map->topic != 0) {
        topic_map->handler(rpc_conf, dataBuf, 0);
        topic_map++;
    }

    os_free(topicBuf);
    os_free(dataBuf);
}


void ICACHE_FLASH_ATTR
MQTTRpc_Init(MQTTRPC_Conf* rpc_conf, MQTT_Client* mqtt_client) {
    // Build topics
    int base_topic_len = os_strlen(BASE_TOPIC) + os_strlen(mqtt_client->connect_info.client_id) + 2;
    char* base_topic = (char*)os_zalloc(base_topic_len);
    os_memset(base_topic, 0, base_topic_len);
    os_strcpy(base_topic, BASE_TOPIC);
    os_strcat(base_topic, "/");
    os_strcat(base_topic, mqtt_client->connect_info.client_id);

    int status_topic_len = base_topic_len + os_strlen(STATUS_TOPIC) + 2;
    char* status_topic = (char*)os_zalloc(status_topic_len);
    os_memset(status_topic, 0, status_topic_len);
    os_strcpy(status_topic, base_topic);
    os_strcat(status_topic, "/");
    os_strcat(status_topic, STATUS_TOPIC);

    int ack_topic_len = base_topic_len + os_strlen(ACK_TOPIC) + 2;
    char* ack_topic = (char*)os_zalloc(ack_topic_len);
    os_memset(ack_topic, 0, ack_topic_len);
    os_strcpy(ack_topic, base_topic);
    os_strcat(ack_topic, "/");
    os_strcat(ack_topic, ACK_TOPIC);

    int spec_topic_len = base_topic_len + os_strlen(SPEC_TOPIC) + 2;
    char* spec_topic = (char*)os_zalloc(spec_topic_len);
    os_memset(spec_topic, 0, spec_topic_len);
    os_strcpy(spec_topic, base_topic);
    os_strcat(spec_topic, "/");
    os_strcat(spec_topic, SPEC_TOPIC);

    rpc_conf->base_topic = base_topic;
    rpc_conf->status_topic = status_topic;
    rpc_conf->ack_topic = ack_topic;
    rpc_conf->spec_topic = spec_topic;
    mqtt_client->user_data = (void*)rpc_conf;

    // Initialize MQTT client
    MQTT_OnConnected(mqtt_client, mqtt_connected_cb);
    MQTT_OnDisconnected(mqtt_client, mqtt_disconnected_cb);
    MQTT_OnPublished(mqtt_client, mqtt_published_cb);
    MQTT_OnData(mqtt_client, mqtt_data_cb);
    // Initialize last will message
    MQTT_InitLWT(
        mqtt_client, (uint8_t*)rpc_conf->status_topic, (uint8_t*)rpc_conf->offline_message,
        rpc_conf->last_will_qos, rpc_conf->last_will_retain);

    MQTT_Connect(mqtt_client);
}
