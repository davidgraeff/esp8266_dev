#include "esp_common.h"
#include "network/simple_udp.h"
#include "ds18b20.h"
#include "driver/gpio.h"
#include "driver/easygpio.h"

const char app_version[] = "1.0\n";
const char default_name[] = "Heizung\n";

char temp_data[20];

#define SET_HEATING(v) GPIO_OUTPUT_SET(GPIO_ID_PIN(3), v)

void simpleudp_init_commands() {
        //PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
        easygpio_pinMode(3, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);

	uint32 rtc_reg_val = 0;
	system_rtc_mem_read(64,&rtc_reg_val,sizeof(uint32));

	strcpy(temp_data, "Temperatur: ");

	add_action("heating", "Heizung", ActTypeToggle, rtc_reg_val>1?1:0, 0, 1);
	add_action("temperature", temp_data, ActTypeStateless, 0, 0, 1000);

	char* t = temp_data + 11;
	init_ds18b20_timer(t,2);
}

void ICACHE_FLASH_ATTR simpleudp_command(struct ActionEntry* entry, int value)
{
    if (strcmp(entry->id, "heating")==0) {
        entry->value = value;
	SET_HEATING(value);
	uint32 rtc_reg_val = value;
	system_rtc_mem_write(64,&rtc_reg_val,sizeof(uint32));
    } else if (strcmp(entry->id, "temperature")==0) {
        print_device_type();
        update_temp(temp_data + 11);
        printf("%s\n", temp_data);
    }
}
