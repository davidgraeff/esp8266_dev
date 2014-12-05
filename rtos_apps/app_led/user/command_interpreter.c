#include "esp_common.h"
#include "network/simple_udp.h"
#include "driver/gpio.h"
#include "driver/espmissingincludes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "config.h"

const char app_version[] = "1.0\n";
const char default_name[] = "Led\n";

static os_timer_t timer_power = {0};

//I just used a scope to figure out the right time periods.


inline void  SEND_WS_0()
{
	uint8_t time;
	time = 3; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	time = 8; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
}

inline void  SEND_WS_1()
{
	uint8_t time;
	time = 7; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	time = 5; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
}

static void ICACHE_FLASH_ATTR WS2812OutBuffer( uint8_t * buffer, uint16_t length )
{
	uint16_t i;
	SET_LED(0);

	vPortEnterCritical();

	for( i = 0; i < length; i++ )
	{
		uint8_t mask = 0x80;
		uint8_t byte = buffer[i];
		while (mask)
		{
			if( byte & mask ) SEND_WS_1(); else SEND_WS_0();
			mask >>= 1;
        }
	}

	vPortExitCritical();
}

uint8_t d[100] = {0};
int c = 0;
static void ICACHE_FLASH_ATTR stop(void *arg) {
    d[c%sizeof(d)] = 0;
    c++;
    d[c%sizeof(d)] = 255;
    WS2812OutBuffer(d, sizeof(d));
}

void ICACHE_FLASH_ATTR simpleudp_init_commands() {
	add_action("led_value", "Led Wert", ActTypeValue, 0, 0, 255);

    //Disarm timer
    os_timer_disarm(&timer_power);

    //Setup and arm timer
    os_timer_setfn(&timer_power, (os_timer_func_t *)stop, 0);
    os_timer_arm(&timer_power, 500, 1);
}

void ICACHE_FLASH_ATTR simpleudp_command(struct ActionEntry* entry, int value)
{
    if (strcmp(entry->id, "led_value")==0) {

    }
}