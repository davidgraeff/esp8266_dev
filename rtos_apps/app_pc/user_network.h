#pragma once

void network_init();
void network_send_broadcast(const char* data, int len);
void network_send(void* destination, const char* data, int len);

// To be implemented by other compilation unit
void network_data_received(void *arg, char *data, unsigned short len);
void network_before_start();