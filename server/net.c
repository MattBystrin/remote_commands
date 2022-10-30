#define _POSIX_C_SOURCE 3
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>

#include <fcntl.h>

#include "net.h"
#include "thread_pool.h"

#define MAX_WAIT_CONN 2

static int epoll_fd = -1;
static int sock_fd = -1;

static int peer_fd = -1;
static FILE *pipe_f = NULL;
static int peers_num = 0;

static void peer_receive(void *args);
static void pipe_receive(void *args);

static handle_t peer_receive_handle = {
	.function = peer_receive,
	.args = NULL
};

static handle_t pipe_receive_handle = {
	.function = pipe_receive,
	.args = NULL
};

static void accept_handle(void *args)
{
	printf("Accept handle\n");
	int sock_fd = *(int *)args;
	peer_fd = accept(sock_fd, NULL, NULL);
	struct epoll_event peer_event = {
		.events = EPOLLIN,
		.data.ptr = &peer_receive_handle
	};
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, peer_fd, &peer_event);
}

static handle_t accept_task = {
	.function = accept_handle,
	.args = &sock_fd
};

int net_init(const char *addr_str, uint16_t port)
{
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	const int enable = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(addr_str);
	int ret = bind(sock_fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (ret) {
		printf("Bind error\n");
	}
	listen(sock_fd, MAX_WAIT_CONN);

	struct epoll_event accept_ev;
	accept_ev.events = EPOLLIN;
	accept_ev.data.ptr = &accept_task;

	epoll_fd = epoll_create(1);
	if (epoll_fd == -1) {
		printf("Error creating epoll\n");
		return -1;
	}
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &accept_ev);

	return sock_fd;
}


static void peer_receive(void *args)
{
	uint32_t msg_len = 0;
	int len = recv(peer_fd, (uint8_t *)&msg_len, sizeof(msg_len), 0);
	if (len == 0) {
		return;
	}
	msg_len = ntohl(msg_len);

	const char *append = " 2>&1";
	printf("Allocating %d bytes %ld\n", msg_len, strlen(append));

	char *buf = malloc(msg_len + strlen(append));
	if (buf == NULL) {
		printf("Error alocating memory for buf\n");
		return;
	}

	len = recv(peer_fd, buf, msg_len, 0);
	if (buf[len - 1] != 0) {
		printf("Corrupted packet %d, %d\n", len, msg_len);
		for (int i = 0; i  < msg_len; i++) {
			printf("%02X ", buf[i]);
		}
		printf("\n");
		free(buf);
		return;
	}
	printf("%s\n", buf);
	strcat(buf, append);
	pipe_f = popen(buf, "r");
	free(buf);
	struct epoll_event pipe_event = {
		.events = EPOLLIN,
		.data.ptr = &pipe_receive_handle
	};
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_f->_fileno, &pipe_event);
}

/* This task probably will be running in multithread */
static void pipe_receive(void *args)
{
	char buf[100];
	int len = read(pipe_f->_fileno, buf, 100);
	if (len == 0) {
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, peer_fd, NULL);
		close(peer_fd);
		peer_fd = -1;
		pclose(pipe_f);
		return;
	}
	len = write(peer_fd, buf, len);
	return;
}


int net_event_loop()
{
	struct epoll_event triggered_events[10];
	int num = epoll_wait(epoll_fd, triggered_events, 10, 0);
	if (num == -1) {
		printf("Epoll error\n");
		return -1;
	}
	for (int i = 0; i < num; i++) {
		if (!triggered_events[i].data.ptr) {
			printf("Warning null event.data.ptr\n");
			continue;
		}
		handle_t handle = *(handle_t *)(triggered_events[i].data.ptr);
		handle.function(handle.args);
	}
	return 0;
}

int net_deinit()
{
	if (close(epoll_fd)) {
		printf("Error closing epoll descriptor\n");
		return -1;
	}
	/* Close clients' sockets */
	close(peer_fd);
	close(sock_fd);
	return 0;
}
