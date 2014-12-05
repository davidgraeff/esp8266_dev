#include "esp_common.h"
#include "driver/espmissingincludes.h"
#include "driver/blinking_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ICACHE_FLASH_ATTR write_uart(void *pvParameters)
{
	while (1)
	{
            printf("SDK version:%s\n", system_get_sdk_version());
	}
}

void ICACHE_FLASH_ATTR user_init(void)
{  
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    init_blinking_led();

    xTaskCreate(write_uart, (const signed char*)"write_uart", 256, NULL, 2, NULL);
}
