#include "esp_common.h"
#include "network/simple_udp.h"
#include "driver/gpio.h"
#include "driver/espmissingincludes.h"

const char app_version[] = "1.0\n";
const char default_name[] = "Rollo\n";

static os_timer_t timer_power = {0};

//#define SET_POWER(v) GPIO_OUTPUT_SET(GPIO_ID_PIN(4), v)
#define SET_POWER(v) {}
#define SET_DIRECTION(v) GPIO_OUTPUT_SET(GPIO_ID_PIN(5), v)

void ICACHE_FLASH_ATTR simpleudp_init_commands() {
	add_action("blind_open", "Öffnen", ActTypeStateless, 0, 0, 0);
	add_action("blind_close", "Schließen", ActTypeStateless, 0, 0, 0);
	add_action("blind_stop", "Stoppen", ActTypeStateless, 0, 0, 0);
}

static void ICACHE_FLASH_ATTR stop(void *arg) {
    os_timer_disarm(&timer_power);
   SET_POWER(0);
printf("SET_POWER 0\n");
}

static void ICACHE_FLASH_ATTR set(uint8_t openess) {
    //Disarm timer
    os_timer_disarm(&timer_power);

    //Setup and arm timer
    os_timer_setfn(&timer_power, (os_timer_func_t *)stop, 0);
    os_timer_arm(&timer_power, 30000, 1);

   if (openess) {
      SET_DIRECTION(1);
printf("SET_DIRECTION 1\n");
   } else {
      SET_DIRECTION(0);
printf("SET_DIRECTION 0\n");
   }
   SET_POWER(1);
printf("SET_POWER 1\n");
}


void ICACHE_FLASH_ATTR simpleudp_command(struct ActionEntry* entry, int value)
{
    if (strcmp(entry->id, "blind_open")==0) {
	set(1);
    } else if (strcmp(entry->id, "blind_close")==0) {
	set(0);
    } else if (strcmp(entry->id, "blind_stop")==0) {
	stop(0);
    }
}