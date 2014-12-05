#include "esp_common.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO GPIO_ID_PIN(2)
#define LED_GPIO_MUX PERIPHS_IO_MUX_GPIO2_U
#define LED_GPIO_FUNC FUNC_GPIO2

LOCAL uint8_t led_state=0;

void ICACHE_FLASH_ATTR led_blink(void *pvParameters)
{
        PIN_FUNC_SELECT(LED_GPIO_MUX, LED_GPIO_FUNC);
	portTickType xLastWakeTime;
	while (1)
	{
		GPIO_OUTPUT_SET(LED_GPIO, led_state);
		led_state ^=1;
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime,(led_state?50:1000)/portTICK_RATE_MS );
	}
}

void init_blinking_led() {
    xTaskCreate(led_blink, (const signed char*)"led_blink", 256, NULL, 2, NULL);
}
