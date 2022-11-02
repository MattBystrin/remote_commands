#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "thread_pool.h"

struct peer {
	int sock_fd;
	FILE *pipe;
	handle_t sock_recv;
	handle_t pipe_recv;
	pthread_mutex_t pipe_mtx; /**/
	pthread_mutex_t sock_mtx; /**/
};

int net_init(struct in_addr addr, uint16_t port, int thread_num);
int net_event_loop();
int net_deinit();

#endif
