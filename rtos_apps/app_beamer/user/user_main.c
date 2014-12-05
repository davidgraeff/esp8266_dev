#include "esp_common.h"
#include "driver/espmissingincludes.h"
#include "driver/gpio.h"
#include "network/user_network.h"

inline void gpio_user_init() {
    // GPIO init
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    GPIO_AS_OUTPUT(GPIO_Pin_2|GPIO_Pin_0);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 0);
}

void ICACHE_FLASH_ATTR user_init(void)
{  
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    gpio_user_init();
    network_init();
}
