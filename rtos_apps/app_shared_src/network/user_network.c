#include "user_network.h"

#include "esp_common.h"
#include "smartconfig.h"
#include "../driver/espmissingincludes.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

struct sockaddr_in receive_addr, broadcast_addr, send_addr;
        int server_sock = -1;

struct config_t {
    struct station_config config;
    uint32_t valid;
} config;
#define PRIV_PARAM_START_SEC            0x3C
#define CONFIG_VALID 0xDEADBEEF

const char* default_ssid = "KloenneDeluxeSlow";
const char* default_pwd = "elch1fahne";

static os_timer_t timer_check_connection = {0};
static os_timer_t timer_dhcp_renew = {0};

const int sendPort = 3339;

static void network_task(void*);

char sendBuffer[100];

void ICACHE_FLASH_ATTR debug_print(void* arg) {
    struct sockaddr_in* addr = (struct sockaddr_in*)arg;
    printf("%s %d",
	inet_ntoa(addr->sin_addr),
	htons(addr->sin_port));
}

static ICACHE_FLASH_ATTR int
save_user_config()
{
	config.valid=CONFIG_VALID;
    spi_flash_erase_sector(PRIV_PARAM_START_SEC);
    spi_flash_write((PRIV_PARAM_START_SEC) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)&config, sizeof(struct config_t));
	return 0;
}

void ICACHE_FLASH_ATTR network_init() {
    network_before_start();
    xTaskCreate(network_task, (const signed char*)"tsk2", 512, NULL, 2, NULL);
}

void ICACHE_FLASH_ATTR network_send_broadcast(const char* data, int len) {
    const int flags = 0;
    if (sendto(server_sock, data, len, flags, (const struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        printf("E > broadcast send fail: ");
	debug_print(&broadcast_addr);
	printf(", %d, %s\n\r", len, data);
    }
}

void ICACHE_FLASH_ATTR network_send(void* destination, const char* data, int len) {
    if (destination == 0) {
        network_send_broadcast(data, len);
        return;
    }

    const int flags = 0;
    struct sockaddr_in* addr = (struct sockaddr_in*)destination;
    addr->sin_port = htons(sendPort); // change port to sendport
    if (sendto(server_sock, data, len, flags, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0) {
        printf("E > send fail: ");
	debug_print(addr);
	printf(", %d, %s\n\r", len, data);
    }
}

int connect_retry = 0;
#define MAX_RETRY_BEFORE_RECONFIGURE 10

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = (struct station_config *)pdata;
            strcpy((char*)config.config.ssid, (char*)sta_conf->ssid);
            strcpy((char*)config.config.password, (char*)sta_conf->password);
            config.config.bssid_set = 0;

            save_user_config();

            connect_retry = 0;

            wifi_station_set_config(sta_conf);
            wifi_station_disconnect();
            wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            uint8 phone_ip[4] = {0};

            memcpy(phone_ip, (uint8*)pdata, 4);
            printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            smartconfig_stop();
            break;
    }
}

static bool ICACHE_FLASH_ATTR check_for_connection()
{
    // We execute this timer function as long as we do not have an IP assigned
    struct ip_info info;
    wifi_get_ip_info(STATION_IF, &info);
    
    uint8 s = wifi_station_get_connect_status();
        
    if (s == STATION_GOT_IP && info.ip.addr != 0) {
        printf(IPSTR "\n\r", IP2STR(&info.ip));
        
        // IP assigned, disarm timer
        os_timer_disarm(&timer_check_connection);
        
        connect_retry = 0;

        return true;
    } else {
        switch (s) {
            case STATION_CONNECTING:
		printf(".\n\r");
		break;
            case STATION_WRONG_PASSWORD:
                connect_retry = MAX_RETRY_BEFORE_RECONFIGURE;
		printf("E > Wrong pass..\n\r");
		break;
            case STATION_NO_AP_FOUND:
                ++connect_retry;
                printf("E > No AP..\n\r");
                wifi_station_disconnect();
                wifi_station_connect();
                break;
            case STATION_CONNECT_FAIL:
                ++connect_retry;
                printf("E > connection fail..\n\r");
                wifi_station_disconnect();
                wifi_station_connect();
                break;
            case STATION_IDLE: default: printf(".\n\r"); break;
        };
    }

    if (connect_retry >= MAX_RETRY_BEFORE_RECONFIGURE) {
        // Smartconfig
        os_timer_disarm(&timer_check_connection);
        smartconfig_start(SC_TYPE_ESPTOUCH, smartconfig_done);//SC_TYPE_AIRKISS
    }
    return false;
}

static void ICACHE_FLASH_ATTR renew_dhcp(void *arg) {
    if (DEBUG) printf("D > Recheck dhcp\n\r");
    if (netif_default)
        dhcp_renew(netif_default);
}

void network_task(void *pvParameters)
{
    //Set station mode
    wifi_set_opmode( STATION_MODE );

    config.valid = CONFIG_VALID;
    spi_flash_read((PRIV_PARAM_START_SEC) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)&config, sizeof(struct config_t));
    if (config.valid != CONFIG_VALID)
    {
        memset(&config, 0, sizeof(struct config_t));
        strcpy((char*)config.config.ssid, default_ssid);
        strcpy((char*)config.config.password, default_pwd);
        config.config.bssid_set = 0;
        save_user_config();
    }

    //Set ap settings
    wifi_station_set_config(&(config.config));
    wifi_station_set_auto_connect(1);

    // Addresses
    bzero(&receive_addr, sizeof(struct sockaddr_in));
    receive_addr.sin_family = AF_INET;
    receive_addr.sin_addr.s_addr = INADDR_ANY;
    receive_addr.sin_port = htons(3338);

    bzero(&broadcast_addr, sizeof(struct sockaddr_in));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
    broadcast_addr.sin_port = htons(sendPort);

    portTickType xLastWakeTime;
    while (1) {
        if (!check_for_connection()) {
            xLastWakeTime = xTaskGetTickCount();
            vTaskDelayUntil(&xLastWakeTime,2000/portTICK_RATE_MS );
            continue;
        }

        if (-1 == (server_sock = socket(AF_INET, SOCK_DGRAM, 0))) {
            printf("E > socket error\n\r");
            return;
        }

        int broadcast = 1;
        if (setsockopt(server_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            printf("E > socket options error\n\r");
            return;
        }

        if (-1 == bind(server_sock, (struct sockaddr *)(&receive_addr), sizeof(struct sockaddr))) {
            printf("E > bind fail\n\r");
            return;
        }

        // Start renew
        //Disarm timer
        os_timer_disarm(&timer_dhcp_renew);

        //Setup and arm timer
        os_timer_setfn(&timer_dhcp_renew, (os_timer_func_t *)renew_dhcp, 0);
        os_timer_arm(&timer_dhcp_renew, 60000*60, 1);

        const int recv_len = 256;
        char *recv_buf = (char *)zalloc(recv_len);
        char recv_addr_b[sizeof(struct sockaddr_in)];
        socklen_t addr_len = sizeof(struct sockaddr_in);

        for (;;) {
            if (DEBUG) printf("D > Wait on port: %d\n\r", ntohs(receive_addr.sin_port));

            int recbytes;
            const int flags = 0;
            while ((recbytes = recvfrom(server_sock, recv_buf, recv_len, flags, (struct sockaddr *)recv_addr_b, &addr_len)) > 0) {
                recv_buf[recbytes] = 0;
                const int t = sizeof(struct sockaddr_in);
                if (addr_len != t) {
                    printf("E > read peer address failed! %d %d\n\r", (int)addr_len, t);
                    continue;
                }
                struct sockaddr_in copy;
                memcpy(&copy, &recv_addr_b, sizeof(struct sockaddr_in));
                network_data_received(&copy, recv_buf, recbytes);
            }

            if (recbytes < 0) {
                printf("E > read data fail!\n\r");
                break;
            }
        }

        // Finish up. Disarm dhcp renew timer
        free(recv_buf);
        close(server_sock);
        server_sock = -1;
        os_timer_disarm(&timer_dhcp_renew);
    }
}