#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "wifi.h"


void ICACHE_FLASH_ATTR
user_init()
{
    // system_set_os_print(0);
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    INFO("%s\n", "Connecting to wifi...");
    WIFI_Connect(WIFI_SSID, WIFI_PASS, 0);

    os_printf("Hello World, Blinking\n\r");
}
