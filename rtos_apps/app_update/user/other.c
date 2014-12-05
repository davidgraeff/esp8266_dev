#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_check_ip(void)
{
    struct ip_info ipconfig;

    os_timer_disarm(&client_timer);

    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (ipconfig.ip.addr != 0) {
    	//user_esp_platform_upgrade_begin();
    } else {
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
        os_timer_arm(&client_timer, 100, 0);
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_init
 * Description  : device parame init based on espressif platform
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_init(void)
{
	struct station_config *config = (struct station_config *) os_zalloc(
					sizeof(struct station_config));
	os_sprintf(config->ssid, AP_SSID);
	os_sprintf(config->password, AP_PASSWORD);

	wifi_station_set_config(config);
	wifi_set_opmode(STATIONAP_MODE);

    if (wifi_get_opmode() != SOFTAP_MODE) {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
        os_timer_arm(&client_timer, 100, 0);
    }
}
