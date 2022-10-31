#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <asm-generic/errno-base.h>
#include <pthread.h>

#include "net.h"
#include "thread_pool.h"
#include "error.h"

static int epoll_fd = -1;
static int sock_fd = -1;
static bool multi_thread = false;

#define MAX_PEERS 6
static struct peer peers[MAX_PEERS];

static void close_conn(struct peer *p);
static void peer_receive(void *args);
static void pipe_receive(void *args);
static void peer_accept(void *args);

static struct peer *find_slot()
{
	struct peer *ret = NULL;
	for (int i = 0; i < MAX_PEERS; i++) {
		if (peers[i].sock_fd < 0) {
			ret = peers + i;
			break;
		}
	}
	return ret;
}
/* Must be protected because changes shared resouces */
/* Not reentrant !!! */
static void close_conn(struct peer *p)
{
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, p->sock_fd, NULL);
	printf("Peer %d closed connection\n", p->sock_fd);
	if (p->pipe)
		pclose(p->pipe);
	close(p->sock_fd);
	p->sock_fd = -1;
}

static handle_t accept_handle = {
	.function = peer_accept,
	.args = NULL
};

static void peer_accept(void *args)
{
	printf("Accept handle %ld\n", pthread_self());
	struct peer *p = find_slot(); /* Find free slot for incoming peer */
	if (!p) {
		printf("No more peer availible\n");
		int tmp = accept(sock_fd, NULL, NULL);
		close(tmp);
		return;
	}
	p->sock_fd = accept(sock_fd, NULL, NULL);
	if (p->sock_fd < 0)
		printf("Error accepting\n");
	p->sock_recv.function = peer_receive;
	p->sock_recv.args = p;
	printf("Accept sockfd %d, %ld\n", p->sock_fd, pthread_self());
	struct epoll_event peer_event = {
		.events = EPOLLIN,
		.data.ptr = &p->sock_recv
	};
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, p->sock_fd, &peer_event);
}

static void peer_receive(void *args)
{
	struct peer *p = (struct peer *)args;
	uint32_t msg_len = 0;
	int len = recv(p->sock_fd, (uint8_t *)&msg_len, sizeof(msg_len), 0);
	if (len == 0) {
		close_conn(p);
		return;
	}
	msg_len = ntohl(msg_len);
	const char *append = " 2>&1";
	char *buf = malloc(msg_len + strlen(append));
	//error(buf,"malloc() error");
	len = recv(p->sock_fd, buf, msg_len, 0);
	if (buf[len - 1] != 0) {
		printf("Corrupted packet\n");
		free(buf);
		return;
	}
	printf("Received command: %s\n", buf);
	strcat(buf, append);
	p->pipe = popen(buf, "r");

	free(buf);

	p->pipe_recv.function = pipe_receive;
	p->pipe_recv.args = p;
	struct epoll_event pipe_event = {
		.events = EPOLLIN,
		.data.ptr = &p->pipe_recv
	};
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, p->pipe->_fileno, &pipe_event);
}

/* This task probably will be running in multithread */
static void pipe_receive(void *args)
{
	struct peer *p = (struct peer *)args;
	printf("Pipe receive event\n");
	char buf[100];
	int len = read(p->pipe->_fileno, buf, 100);
	if (len == 0) {
		close_conn(p);
		return;
	}
	len = send(p->sock_fd, buf, len, MSG_NOSIGNAL);
	return;
}


static int init_socket(const char *addr_str, uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	const int en = 1;
	int ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));
	//error(!ret, "setsockopt()");
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(addr_str);
	ret = bind(sock, (const struct sockaddr *)&addr, sizeof(addr));
	//error(!ret, "bind()");
	ret = listen(sock, MAX_PEERS);
	//error(!ret, "listen()");
	return sock;
}

int net_init(const char *addr_str, uint16_t port, int thread_num)
{
	for (int i = 0; i < MAX_PEERS; i++) {
		peers[i].sock_fd = -1;
		peers[i].pipe = NULL;
	}

	sock_fd = init_socket(addr_str, port);

	struct epoll_event accept_ev;
	accept_ev.events = EPOLLIN;
	accept_ev.data.ptr = &accept_handle;

	epoll_fd = epoll_create(1);
	if (epoll_fd == -1) {
		printf("Error creating epoll\n");
		return -1;
	}
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &accept_ev);

	if (thread_num > 0) {
		multi_thread = true;
		allocate_pool(thread_num);
	}

	return 0;
}

int net_event_loop()
{
	struct epoll_event triggered_events[10];
	int num = epoll_wait(epoll_fd, triggered_events, 10, -1);
	if (num == -1) {
		printf("epoll() error %s\n", strerror(errno));
		return -1;
	}
	printf("Events occured %d %ld\n", num, pthread_self());
	for (int i = 0; i < num; i++) {
		if (!triggered_events[i].data.ptr) {
			printf("NULL args event\n");
			continue;
		}
		handle_t handle = *(handle_t *)(triggered_events[i].data.ptr);
		if (multi_thread) {
			printf("Pushing task %ld\n", pthread_self());
			push_task(handle);
		} else {
			exec_task(handle);
		}
	}
	/* This prevents reenter of one task for one socket or pipe */
	if (multi_thread)
		while(!tasks_ready()); /* Can be a condvar */
	return 0;
}

int net_deinit()
{
	if (multi_thread)
		finish_pool();
	int ret = close(epoll_fd);
	/* Close clients' sockets */
	for (int i = 0; i < MAX_PEERS; i++) {
		if (peers[i].sock_fd)
			close(peers[i].sock_fd);
	}
	close(sock_fd);
	return 0;
}
