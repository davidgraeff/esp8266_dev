#include "driver/espmissingincludes.h"
#include "driver/easygpio.h"
#include "network/user_network.h"

inline void gpio_user_init() {
    easygpio_pinMode(0, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
    easygpio_pinMode(2, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
}

void ICACHE_FLASH_ATTR user_init(void)
{  
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    gpio_user_init();
    network_init();
}
