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
}



const MQTTRPC_Topic_Map topics_map[] = {
        { .topic = "/my/cool/topic", .handler = simple_handler, .qos = 0},
        { .topic = "/my/+/topic", .handler = simple_handler, .qos = 0},
        { .topic = "/my/cool/+", .handler = simple_handler, .qos = 0},
        { .topic = "/+/cool/topic", .handler = simple_handler, .qos = 0},
        { .topic = "+/cool/topic", .handler = simple_handler, .qos = 0},
        { .topic = 0 }
    };
MQTTRPC_Conf rpc_conf = MQTTRPC_INIT_CONF( .handlers = topics_map);



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
