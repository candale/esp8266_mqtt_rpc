#include "mqtt_rpc.h"


static void ICACHE_FLASH_ATTR
mqtt_connected_cb(uint32_t *args)
{
    RPC_INFO("MQTTRPC: MQTT client connected\r\n");

    MQTT_Client* mqtt_client = (MQTT_Client*)args;
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)mqtt_client->user_data;
    const MQTTRPC_Topic_Map* handlers = rpc_conf->handlers;

    // Subscribe for all handlers
    while(handlers->topic != 0) {
        MQTT_Subscribe(mqtt_client, handlers->topic, handlers->qos);
        handlers++;
    }

    // TODO: publish all specs
    MQTT_Publish(
        mqtt_client, rpc_conf->status_topic, rpc_conf->online_message,
        os_strlen(rpc_conf->online_message), rpc_conf->online_qos,
        rpc_conf->online_retain);
}


static void ICACHE_FLASH_ATTR
mqtt_disconnected_cb(uint32_t *args)
{
    RPC_INFO("MQTTRPC: MQTT client disconnected\r\n");
    MQTT_Client* client = (MQTT_Client*)args;
}


static void ICACHE_FLASH_ATTR
mqtt_published_cb(uint32_t *args)
{
    RPC_INFO("MQTTRPC: MQTT message published\r\n");

    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}


uint8_t number_of_occurrs(char* str, char to_find)
{
    uint8_t count = 0;
    int i;
    for(i = 0; i < os_strlen(str); i++) {
        if(str[i] == to_find) {
            count ++;
        }
    }
    return count;
}


uint8_t chars_until(char* str, char stop_char)
{
    int count = 0;
    while(str[count] != stop_char && count < os_strlen(str)) {
        count ++;
    }

    return count;
}


uint8_t match_topic(char* template, char* source, char* args[])
{
    if(number_of_occurrs(template, '/') != number_of_occurrs(source, '/')) {
        return 0;
    }

    char prev_tmpl_char = 0, args_count = 0;
    int tmpl_i = 0, src_i = 0;
    int tmpl_len = os_strlen(template);
    int src_len = os_strlen(source);

    do {
        uint8_t is_true_wildcard = (
            (prev_tmpl_char == 0 || prev_tmpl_char == '/') &&
            (tmpl_i + 1 < tmpl_len  && template[tmpl_i + 1] == '/') ||
            tmpl_i == tmpl_len - 1
        );
        if(template[tmpl_i] == '+' && is_true_wildcard) {
            int param_len = chars_until(source + src_i, '/');
            char* param = (char*)os_malloc(param_len + 1);
            os_memset(param, 0, param_len + 1);
            os_strncpy(param, source + src_i, param_len);
            args[args_count++] = param;

            src_i += param_len - 1;
        } else if(template[tmpl_i] != source[src_i]) {
            return 0;
        }

        prev_tmpl_char = template[tmpl_i];

        src_i ++;
        tmpl_i ++;
    } while(tmpl_i < tmpl_len && src_i < src_len);

    if(tmpl_i != tmpl_len || src_i != src_len) {
        return 0;
    }

    return 1;
}


static void ICACHE_FLASH_ATTR
mqtt_data_cb(uint32_t *args, const char* topic, uint32_t topic_len,
             const char *data, uint32_t data_len)
{
    RPC_INFO("MQTTRPC: MQTT client got data\r\n");
    char* topic_buf = (char*)os_zalloc(topic_len + 1);
    char* data_buf = (char*)os_zalloc(data_len + 1);

    MQTT_Client* client = (MQTT_Client*)args;
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)client->user_data;
    const MQTTRPC_Topic_Map* topic_map = rpc_conf->handlers;

    os_memcpy(topic_buf, topic, topic_len);
    topic_buf[topic_len] = 0;
    os_memcpy(data_buf, data, data_len);
    data_buf[topic_len] = 0;

    RPC_INFO("Received on topic: %s with data %s\r\n", topic_buf, data_buf);

    while(topic_map->topic != 0) {
        uint8_t no_args = number_of_occurrs(topic_map->topic, '+');
        char* args[no_args];

        uint8_t is_matched = match_topic(topic_map->topic, topic_buf, args);

        if(is_matched) {
            RPC_INFO("Matched handler with topic %s\r\n", topic_map->topic);

            topic_map->handler(rpc_conf, data_buf, args, no_args);
        }

        RPC_INFO("Freeing data\r\n");
        int i;
        for(i = 0; i < no_args; i++) {
            os_free(args[i]);
        }

        topic_map++;
    }

    os_free(topic_buf);
    os_free(data_buf);
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
