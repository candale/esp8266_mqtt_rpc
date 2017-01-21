#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "wifi.h"
#include "mqtt.h"
#include "mem.h"
#include "mqtt_rpc.h"
#include "user_interface.h"


MQTT_Client mqttClient;


void ICACHE_FLASH_ATTR print_info()
{
  INFO("\r\n\r\n[INFO] BOOTUP...\r\n");
  INFO("[INFO] SDK: %s\r\n", system_get_sdk_version());
  INFO("[INFO] Chip ID: %08X\r\n", system_get_chip_id());
  INFO("[INFO] Memory info:\r\n");
  system_print_meminfo();
}


int ICACHE_FLASH_ATTR
simple_handler(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count)
{
    INFO("IN handler simple_handler with data %s\n", data);
    int i = 0;
    for(i = 0; i < arg_count; i++) {
        INFO("ARG %s\n", args[i]);
    }
    INFO("Available heap %d\n", system_get_free_heap_size());
    // MQTTRPC_Publish(rpc_conf, "other", "this data", 9, 0, 0);
}

int ICACHE_FLASH_ATTR
all_handler(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count)
{
    INFO("IN handler all_handler with data %s\n", data);
}

int ICACHE_FLASH_ATTR
other_handler(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count)
{
    INFO("IN handler other_handler with data %s\n", data);
}

int ICACHE_FLASH_ATTR
hallway_light(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler hallway_light\r\n");
}

int ICACHE_FLASH_ATTR
livingroom_tv(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler livingroom_tv\r\n");
}

int ICACHE_FLASH_ATTR
beehive(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler beehive\r\n");
}

int ICACHE_FLASH_ATTR
beehive_heating(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler beehive_heating\r\n");
}

int ICACHE_FLASH_ATTR
work_garage(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler work_garage\r\n");
}

int ICACHE_FLASH_ATTR
work_gate(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler work_gate\r\n");
}

int ICACHE_FLASH_ATTR
gatekeeper_garage(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler gatekeeper_garage\r\n");
}

int ICACHE_FLASH_ATTR
gatekeeper_gate(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler gatekeeper_gate\r\n");
}

int ICACHE_FLASH_ATTR
home_desktop(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler home_desktop\r\n");
}

int ICACHE_FLASH_ATTR
home_basement_heating(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler home_basement_heating\r\n");
}

int ICACHE_FLASH_ATTR
long_handler(MQTTRPC_Conf* rpc_conf, char* data, char* args[], uint8_t arg_count) {
    INFO("In handler long_handler\r\n");
}


void ICACHE_FLASH_ATTR
rpc_connected(uint32_t* args) {
    MQTTRPC_Conf* rpc_conf = (MQTTRPC_Conf*)args;

    INFO("In RPC connected callback\n");
}

const MQTTRPC_Topic_Map topics_map[] = {
        { .topic = "", .handler = all_handler},
        { .topic = "other",                     .handler = other_handler},
        { .topic = "my/+/topic",                .handler = simple_handler, .qos = 0},
        { .topic = "hallway/light1/+",          .handler = hallway_light, .qos = 0},
        { .topic = "livingroom/tv/+",           .handler = livingroom_tv, .qos = 0},
        { .topic = "beehive/+/+",               .handler = beehive, .qos = 0},
        { .topic = "beehive/heating/+",         .handler = beehive_heating, .qos = 0},
        { .topic = "work/garage",               .handler = work_garage, .qos = 0},
        { .topic = "work/gate",                 .handler = work_gate, .qos = 0},
        { .topic = "gatekeeper/garage",         .handler = gatekeeper_garage, .qos = 0},
        { .topic = "gatekeeper/gate",           .handler = gatekeeper_gate, .qos = 0},
        { .topic = "home/+/Desktop/+/+/+/+",    .handler = home_desktop, .qos = 0},
        { .topic = "home/basement/heating/+",   .handler = home_basement_heating, .qos = 0},
        { .topic = "a/very/very/very/very/long/topic/that/we/hope/will/work/and/a/+", .handler = long_handler, .qos = 0},
        { .topic = 0 }
    };
MQTTRPC_Conf rpc_conf = MQTTRPC_INIT_CONF(
    .handlers = topics_map, .connected_cb = rpc_connected);



void ICACHE_FLASH_ATTR
init_mqtt_rpc(uint8_t status) {
    if(status != STATION_GOT_IP) {
        return;
    }

    INFO("Initialize MQTT client\r\n");

    MQTT_InitConnection(&mqttClient, MQTT_HOST, MQTT_PORT, DEFAULT_SECURITY);
    if (!MQTT_InitClient(&mqttClient, MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, MQTT_KEEPALIVE, MQTT_CLEAN_SESSION) )
    {
        INFO("Failed to initialize properly. Check MQTT version.\n\r");
        return;
    }
    MQTTRpc_Init(&rpc_conf, &mqttClient);
}

void ICACHE_FLASH_ATTR
init_all(void) {
    // system_set_os_print(0);
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    print_info();

    INFO("Connecting to wifi...\n");
    WIFI_Connect(WIFI_SSID, WIFI_PASS, init_mqtt_rpc);
}

void ICACHE_FLASH_ATTR
user_init()
{
    system_init_done_cb(init_all);
}
