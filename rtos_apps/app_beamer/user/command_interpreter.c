#include "esp_common.h"
#include "network/simple_udp.h"
#include "driver/gpio.h"

const char app_version[] = "1.0\n";
const char default_name[] = "Beamer\n";

/// Write data to pin 2 with 19200 baudrate
int softuart_write( const char* data, unsigned len );

void simpleudp_init_commands() {
	add_action("beamer", "Beamer", ActTypeToggle, 0, 0, 1);
}

void ICACHE_FLASH_ATTR simpleudp_command(struct ActionEntry* entry, int value)
{
    if (strcmp(entry->id, "beamer")==0) {
        entry->value = value;
	if  (entry->value) {
		softuart_write("C00\r", 4);
	} else {
		softuart_write("C01\r", 4);
	}
    }
}