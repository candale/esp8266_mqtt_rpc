#ifndef __USER_CONFIG_H_
#define __USER_CONFIG_H_

#define DEBUG_ON 1

#define WIFI_SSID "ssid"
#define WIFI_PASS "pass"


#if defined(DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#endif
