#include "user_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

struct sockaddr_in receive_addr, broadcast_addr, send_addr;
        int server_sock = -1;

const int sendPort = 3339;

static void network_udp_start(void);

char sendBuffer[100];

void init_check_timer();

void  debug_print(void* arg) {
    struct sockaddr_in* addr = (struct sockaddr_in*)arg;
    printf("%s %d",
	inet_ntoa(addr->sin_addr),
	htons(addr->sin_port));
}

void  network_init() {
    network_before_start();
    network_udp_start();
}

void  network_send_broadcast(const char* data, int len) {
    const int flags = 0;
    if (sendto(server_sock, data, len, flags, (const struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        printf("E > broadcast send fail: ");
	debug_print(&broadcast_addr);
	printf(", %d, %s\n\r", len, data);
    }
}

void  network_send(void* destination, const char* data, int len) {
    if (destination == 0) {
        network_send_broadcast(data, len);
        return;
    }

    const int flags = 0;
    struct sockaddr_in* addr = (struct sockaddr_in*)destination;
    addr->sin_port = htons(sendPort); // change port to sendport
    if (sendto(server_sock, data, len, flags, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0) {
        printf("E > send fail: ");
	debug_print(addr);
	printf(", %d, %s\n\r", len, data);
    }
}

long timevaldiff(struct timeval *starttime, struct timeval *finishtime)
{
  long msec;
  msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
  msec+=(finishtime->tv_usec-starttime->tv_usec)/1000;
  return msec;
}

static void  network_udp_start(void)
{   
    bzero(&receive_addr, sizeof(struct sockaddr_in));
    receive_addr.sin_family = AF_INET;
    receive_addr.sin_addr.s_addr = INADDR_ANY;
    receive_addr.sin_port = htons(3338);

    bzero(&broadcast_addr, sizeof(struct sockaddr_in));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
    broadcast_addr.sin_port = htons(sendPort);

    if (-1 == (server_sock = socket(AF_INET, SOCK_DGRAM, 0))) {
        printf("E > socket error\n\r");
        return;
    }

    int broadcast = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        printf("E > socket options error\n\r");
        return;
    }

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        printf("E > socket options error\n\r");
        return;
    }

    if (-1 == bind(server_sock, (struct sockaddr *)(&receive_addr), sizeof(struct sockaddr))) {
        printf("E > bind fail\n\r");
        return;
    }

    const int recv_len = 256;
    char *recv_buf = (char *)malloc(recv_len);
    char recv_addr_b[sizeof(struct sockaddr_in)];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct timeval system_time, compare_time;

    const char packet_type_detect[] = "SimpleUDP_detect\nOWN\n";

    // We have been started right now: Send broacast message that we are on
    network_data_received(0, (char*)packet_type_detect, sizeof(packet_type_detect)-1);

    if (DEBUG) printf("D > Wait on port: %d\n\r", ntohs(receive_addr.sin_port));
    int recbytes;
    const int flags = 0;

    for (;;) {
        gettimeofday(&system_time, 0);
        errno = 0;
        while ((recbytes = recvfrom(server_sock, recv_buf, recv_len, flags, (struct sockaddr *)recv_addr_b, &addr_len)) > 0) {
            recv_buf[recbytes] = 0;
	    const int t = sizeof(struct sockaddr_in);
	    if (addr_len != t) {
		printf("E > read peer address failed! %d %d\n\r", (int)addr_len, t);
		continue;
	    }
	    struct sockaddr_in copy;
	    memcpy(&copy, &recv_addr_b, sizeof(struct sockaddr_in));
            network_data_received(&copy, recv_buf, recbytes);
        }

        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            gettimeofday(&compare_time, 0);
            if (timevaldiff(&system_time, &compare_time) > 5000) {
                if (DEBUG) printf("D > Resume from suspend\n\r");
                // We have been suspended (system time differs more than 5 seconds): Send broacast message that we are on again
                network_data_received(0, (char*)packet_type_detect, sizeof(packet_type_detect)-1);
            }
            continue;
        }

        if (recbytes < 0) {
            perror("E > read data fail: ");
            break;
        }
    }
    free(recv_buf);
    close(server_sock);
    server_sock = -1;
}
