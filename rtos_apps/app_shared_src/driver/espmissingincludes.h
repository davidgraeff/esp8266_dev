
// https://github.com/mziwisky/esp8266-dev/blob/master/esphttpd/include/espmissingincludes.h

#ifndef ESPMISSINGINCLUIDES_H
#define ESPMISSINGINCLUIDES_H

#include "esp_common.h"

void uart_div_modify(int no, unsigned int freq);

int skip_atoi(const char **nptr);
int atoi(const char *nptr);

void ets_wdt_enable(void);
void ets_wdt_disable(void);
void wdt_feed(void);

//void os_timer_arm(os_timer_t *a, int b, int isMstimer);
void os_timer_disarm(os_timer_t*);
//void os_timer_setfn(os_timer_t *t, void *fn, void *parg);

#endif
