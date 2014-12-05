/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_esp_platform.c
 *
 * Description: The client mode configration.
 *              Check your hardware connection with the host while use this mode.
 *
 * Modification history:
 *     2014/5/09, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "upgrade.h"



#define ESP_DEBUG

#ifdef ESP_DEBUG
#define ESP_DBG os_printf
#else
#define ESP_DBG
#endif


#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

LOCAL os_timer_t client_timer;

void user_esp_platform_check_ip(void);

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    if (server->upgrade_flag == true) {
    	ESP_DBG("user_esp_platform_upgrade_successful\n");

    } else {
        ESP_DBG("user_esp_platform_upgrade_failed\n");

    }

    os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_begin
 * Description  : Processing the received data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                server -- upgrade param
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
upgrade_begin()
{
    struct espconn *pespconn = NULL;
    struct upgrade_server_info *server = NULL;
    server = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));

    uint8 user_bin[21] = {0};


    server->pespconn = pespconn;
    server->port = 80;
    server->check_cb = user_esp_platform_upgrade_rsp;
    server->check_times = 120000;
    const char esp_server_ip[4] = {52,68,48,85};
    os_memcpy(server->ip, esp_server_ip, 4);

    if (server->url == NULL) {
        server->url = (uint8 *)os_zalloc(512);
    }

    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        os_memcpy(user_bin, "user2.4096.new.6.bin", sizeof(user_bin));
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        os_memcpy(user_bin, "user1.4096.new.6.bin", sizeof(user_bin));
    }
    
    os_sprintf(server->url, "GET /download/ota_esp8266/%s HTTP/1.0\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
           user_bin, IP2STR(server->ip),
           80);

    if (system_upgrade_start(server) == false) {

        ESP_DBG("upgrade is already started\n");
    }
}

