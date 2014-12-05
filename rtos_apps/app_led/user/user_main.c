#include "esp_common.h"
#include "driver/espmissingincludes.h"
#include "driver/easygpio.h"
#include "driver/gpio.h"
#include "driver/blinking_led.h"
#include "network/user_network.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"

inline void gpio_user_init() {
    easygpio_pinMode(WSGPIO, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
    //easygpio_pinMode(5, EASYGPIO_NOPULL, EASYGPIO_OUTPUT);
}

void ICACHE_FLASH_ATTR user_init(void)
{  
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    gpio_user_init();

    printf("SDK version:%s\n", system_get_sdk_version());

    //init_blinking_led();

    network_init();
}
