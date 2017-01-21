#include "mqtt_rpc.h"


static void ICACHE_FLASH_ATTR
mqtt_connected_cb(uint32_t *args)
{
    /*
    If a handler has an empty topic, it will be called on topic `<base_topic>/`
    */
    RPC_INFO("MQTTRPC: MQTT client connected\r\n");

    MQTT_Client* mqtt_client = (MQTT_Client*)args;
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)mqtt_client->user_data;
    const MQTTRPC_Topic_Map* handlers = rpc_conf->handlers;

    // Subscribe for all handlers
    while(handlers->topic != 0) {
        char* sub_topic = (char*)os_zalloc(
            os_strlen(rpc_conf->base_topic) + os_strlen(handlers->topic) + 2);
        os_strcpy(sub_topic, rpc_conf->base_topic);
        os_strcat(sub_topic, "/");
        os_strcat(sub_topic, handlers->topic);

        MQTT_Subscribe(mqtt_client, sub_topic, handlers->qos);

        // TODO: don't reallocate memory each time, recycle
        os_free(sub_topic);

        handlers++;
    }

    // TODO: publish all specs
    // Publish that we are online
    MQTT_Publish(
        mqtt_client, rpc_conf->status_topic, rpc_conf->online_message,
        os_strlen(rpc_conf->online_message), rpc_conf->online_qos,
        rpc_conf->online_retain);

    // Call user `connected` callback
    if(rpc_conf->connected_cb) {
        rpc_conf->connected_cb((uint32_t*)rpc_conf);
    }
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
    // TODO: apply same logic as for is_true_wildcard for +
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
    // TODO: optimize for topics that do not have args
    // Treat some simple cases first
    if(os_strlen(template) == os_strlen(source) && os_strlen(source) == 0) {
        return 1;
    }

    if(number_of_occurrs(template, '/') != number_of_occurrs(source, '/')) {
        return 0;
    }

    uint8_t matched = 1;
    char prev_tmpl_char = 0, args_count = 0;
    int tmpl_i = 0, src_i = 0, i;
    int tmpl_len = os_strlen(template);
    int src_len = os_strlen(source);


    // Go through both topics one char after the other and compare them
    do {
        // Consider + a wildcard only if it is one of the following cases
        // '/+', '/+/', '+/'
        uint8_t is_true_wildcard = (
            // make sure it's the beginning of the string or the last char was '/'
            (prev_tmpl_char == 0 || prev_tmpl_char == '/') &&
            (
                // it's not the end of the string and the next char is '/'
                (tmpl_i + 1 < tmpl_len && template[tmpl_i + 1] == '/') ||
                // it's the end of the string
                tmpl_i == tmpl_len - 1
            )
        );

        if(template[tmpl_i] == '+' && is_true_wildcard) {
            // save the parameter from source topic
            int param_len = chars_until(source + src_i, '/');
            char* param = (char*)os_zalloc(param_len + 1);
            os_strncpy(param, source + src_i, param_len);
            args[args_count++] = param;

            src_i += param_len - 1;
        } else if(template[tmpl_i] != source[src_i]) {
            matched = 0;
        }

        prev_tmpl_char = template[tmpl_i];

        src_i ++;
        tmpl_i ++;
    } while(tmpl_i < tmpl_len && src_i < src_len);

    // by the end of the comparison both indexes must have reached their limit
    if(tmpl_i != tmpl_len || src_i != src_len) {
        matched = 0;
    }

    // free the the args if not matched. if there was a match, than it is
    // the callers job to free the args
    if(matched == 0) {
        for(i = 0; i < args_count; i++) {
            os_free(args[i]);
        }
    }

    return matched;
}


static void ICACHE_FLASH_ATTR
mqtt_data_cb(uint32_t *args, const char* topic, uint32_t topic_len,
             const char *data, uint32_t data_len)
{
    // TODO: add support for # wildcard
    RPC_INFO("MQTTRPC: MQTT client got data\r\n");

    int i;
    char* topic_buf = (char*)os_zalloc(topic_len + 1);
    char* data_buf = (char*)os_zalloc(data_len + 1);

    // get required structures
    MQTT_Client* client = (MQTT_Client*)args;
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)client->user_data;
    const MQTTRPC_Topic_Map* topic_map = rpc_conf->handlers;

    os_memcpy(topic_buf, topic, topic_len);
    topic_buf[topic_len] = 0;
    os_memcpy(data_buf, data, data_len);
    data_buf[topic_len] = 0;

    RPC_INFO("MQTTRPC: Received on topic: %s with data %s\r\n", topic_buf, data_buf);

    // If the topic is shorter than our base topic, no reason to go further
    uint8_t should_try_to_match = 1;
    char* to_match;
    if(os_strlen(topic_buf) < os_strlen(rpc_conf->base_topic)) {
        should_try_to_match = 0;
        RPC_INFO("MQTTRPC: Received topic shorter than base\r\n");
    } else {
        // Try to match only the part after base_topic, without the trailing / after it
        to_match = topic_buf + os_strlen(rpc_conf->base_topic);
        if(os_strlen(to_match) != 0 && to_match[0] == '/') {
            to_match ++;
        }
        RPC_INFO("MQTTRPC: Topic to match: '%s'\r\n", to_match);
    }

    while(topic_map->topic != 0 && should_try_to_match) {
        // Number of arguments based on number of + where used as wildcard
        uint8_t no_args = number_of_occurrs(topic_map->topic, '+');
        char* args[no_args];

        uint8_t is_matched = match_topic(topic_map->topic, to_match, args);

        if(is_matched) {
            RPC_INFO("MQTTRPC: Matched handler with topic %s\r\n", topic_map->topic);

            topic_map->handler(rpc_conf, data_buf, args, no_args);

            RPC_INFO("Called handler, doing cleanup\r\n");

            for(i = 0; i < no_args; i++) {
                os_free(args[i]);
            }

            RPC_INFO("Did cleanup\r\n");

            if(rpc_conf->break_on_first) {
                RPC_INFO("BREAKED\r\n");
                break;
            }
        }

        topic_map++;
    }

    RPC_INFO("FREEING topic\r\n");
    os_free(topic_buf);
    RPC_INFO("FREEING data\r\n");
    os_free(data_buf);
}


MQTTRPC_OnConnected(MQTTRPC_Conf* rpc_conf, MqttCallback connected_cb) {
    rpc_conf->connected_cb = connected_cb;
}


void ICACHE_FLASH_ATTR
MQTTRpc_Init(MQTTRPC_Conf* rpc_conf, MQTT_Client* mqtt_client) {
    // Build topics
    int base_topic_len = os_strlen(BASE_TOPIC) + os_strlen(mqtt_client->connect_info.client_id) + 2;
    char* base_topic = (char*)os_zalloc(base_topic_len);
    os_strcpy(base_topic, BASE_TOPIC);
    os_strcat(base_topic, "/");
    os_strcat(base_topic, mqtt_client->connect_info.client_id);

    int status_topic_len = base_topic_len + os_strlen(STATUS_TOPIC) + 2;
    char* status_topic = (char*)os_zalloc(status_topic_len);
    os_strcpy(status_topic, base_topic);
    os_strcat(status_topic, "/");
    os_strcat(status_topic, STATUS_TOPIC);

    int ack_topic_len = base_topic_len + os_strlen(ACK_TOPIC) + 2;
    char* ack_topic = (char*)os_zalloc(ack_topic_len);
    os_strcpy(ack_topic, base_topic);
    os_strcat(ack_topic, "/");
    os_strcat(ack_topic, ACK_TOPIC);

    int spec_topic_len = base_topic_len + os_strlen(SPEC_TOPIC) + 2;
    char* spec_topic = (char*)os_zalloc(spec_topic_len);
    os_strcpy(spec_topic, base_topic);
    os_strcat(spec_topic, "/");
    os_strcat(spec_topic, SPEC_TOPIC);

    rpc_conf->base_topic = base_topic;
    rpc_conf->status_topic = status_topic;
    rpc_conf->ack_topic = ack_topic;
    rpc_conf->spec_topic = spec_topic;

    mqtt_client->user_data = (void*)rpc_conf;
    rpc_conf->_mqtt_client = mqtt_client;

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


uint8_t ICACHE_FLASH_ATTR
MQTTRPC_Publish(MQTTRPC_Conf* config, const char* topic, const char* data,
                int data_len, int qos, int retain)
{
    char* pub_topic = (char*)os_zalloc(
        os_strlen(config->base_topic) + os_strlen(topic) + 2);
    os_strcpy(pub_topic, config->base_topic);
    os_strcat(pub_topic, "/");
    os_strcat(pub_topic, topic);

    uint8_t result = MQTT_Publish(
        config->_mqtt_client, pub_topic, data, data_len + os_strlen(topic), qos, retain);

    os_free(pub_topic);

    return result;
}
