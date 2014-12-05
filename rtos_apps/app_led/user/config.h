#pragma once

#define WSGPIO 2
#define SET_LED(v) GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), v)
