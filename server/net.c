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
#include "messages.h"

/* Module globar variables */
static int epoll_fd = -1;
static int sock_fd = -1;
static bool multi_thread = false;

#define MAX_PEERS 6
static struct peer peers[MAX_PEERS];

static void close_conn(struct peer *p);
static void peer_receive(void *args);
static void pipe_receive(void *args);
static void peer_accept(void *args);

/* Find a free peer slot in peers array */
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

/* Closes connection */
static void close_conn(struct peer *p)
{
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, p->sock_fd, NULL);
	if (ret)
		err(errno, "EPOLL_CTL_DEL peer socket %d", p->sock_fd);
	printf("Peer %d closed connection\n", p->sock_fd);
	if (p->pipe)
		pclose(p->pipe);
	ret = close(p->sock_fd);
	if (ret)
		warn(errno, "Peer socket close failed");
	p->sock_fd = -1;
}

static handle_t accept_handle = {
	.function = peer_accept,
	.args = NULL
};

/* Callback invoked when incoming connection can be accepted */
static void peer_accept(void *args)
{
	struct peer *p = find_slot(); /* Find free slot for incoming peer */
	if (!p) {
		printf("No more peer slots availible\n");
		int tmp = accept(sock_fd, NULL, NULL);
		close(tmp);
		return;
	}
	p->sock_fd = accept(sock_fd, NULL, NULL);
	if (p->sock_fd < 0)
		err(errno, "Failed to accept peer connection");

	p->sock_recv.function = peer_receive;
	p->sock_recv.args = p;
	struct epoll_event peer_event = {
		.events = EPOLLIN,
		.data.ptr = &p->sock_recv
	};
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, p->sock_fd, &peer_event);
	if (ret)
		err(errno, "EPOLL_CTL_ADD peer socket %d", p->sock_fd);
}

/* Callback invoked when data at peer socket availible to read */
static void peer_receive(void *args)
{
	struct peer *p = (struct peer *)args;
	uint32_t msg_len = 0;
	int len = recv(p->sock_fd, (uint8_t *)&msg_len, sizeof(msg_len), 0);
	if (len < 0)
		err(errno, "Receiving from peer %d", p->sock_fd);
	if (len == 0) {
		close_conn(p);
		return;
	}

	msg_len = ntohl(msg_len);
	const char *append = " 2>&1";
	char *buf = malloc(msg_len + strlen(append));
	if (!buf)
		err(0, "failed to allocate memory");

	len = recv(p->sock_fd, buf, msg_len, 0);
	if (len < 0)
		err(errno, "Receiving command from peer %d", p->sock_fd);
	if (buf[len - 1] != 0) {
		printf("Corrupted packet\n");
		free(buf);
		return;
	}
	printf("Received command: %s Peer: %d\n", buf, p->sock_fd);

	strcat(buf, append);
	p->pipe = popen(buf, "r");
	free(buf);
	if (!p->pipe)
		warn(errno, "failed popen()");

	p->pipe_recv.function = pipe_receive;
	p->pipe_recv.args = p;
	struct epoll_event pipe_event = {
		.events = EPOLLIN,
		.data.ptr = &p->pipe_recv
	};
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, p->pipe->_fileno, &pipe_event);
	if (ret)
		err(errno, "EPOLL_CTL_ADD pipe %d peer %d", p->pipe->_fileno, p->sock_fd);
}

/* Callback invoked when data at pipe availible to read */
static void pipe_receive(void *args)
{
	struct peer *p = (struct peer *)args;
	char buf[100];
	int len = read(p->pipe->_fileno, buf, 100);
	if (len < 0)
		err(errno, "Failed to read from pipe %d peer %d",
		    p->pipe->_fileno, p->sock_fd);
	if (len == 0) {
		close_conn(p);
		return;
	}
	len = send(p->sock_fd, buf, len, MSG_NOSIGNAL);
	if (len < 0)
		warn(errno, "Failed send to peer %d", p->sock_fd);
	return;
}

/* Creates socket bind it and switch to listen mode */
static int init_socket(struct in_addr addr, uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		err(errno, "Creating socket");
	const int en = 1;

	int ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));
	if (ret)
		err(errno, "Setting socket option");

	struct sockaddr_in _addr;
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
	_addr.sin_addr = addr;
	ret = bind(sock, (const struct sockaddr *)&_addr, sizeof(_addr));
	if (ret)
		err(errno, "Bind socket");

	ret = listen(sock, MAX_PEERS);
	if (ret)
		err(errno, "Listen socket");
	return sock;
}

/* Init network part */
int net_init(struct in_addr addr, uint16_t port, int thread_num)
{
	for (int i = 0; i < MAX_PEERS; i++) {
		peers[i].sock_fd = -1;
		peers[i].pipe = NULL;
	}

	sock_fd = init_socket(addr, port);

	struct epoll_event accept_ev;
	accept_ev.events = EPOLLIN;
	accept_ev.data.ptr = &accept_handle;

	epoll_fd = epoll_create(1);
	if (epoll_fd < 0)
		err(errno, "Failed creating epoll descriptor");

	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &accept_ev);
	if (ret < 0)
		err(errno, "EPOLL_CTL_ADD socket %d", sock_fd);

	if (thread_num > 0) {
		multi_thread = true;
		allocate_pool(thread_num);
	} else {
		printf("Working in single-thread mode\n");
	}

	return 0;
}

/* 
 * Network event loop. If any events occured in tracked objects (sockets, pipes)
 * corresponding event handler is called. Events can be processed simulaneously
 * or in "parrallel". True parrallel processing comes in only when events occures 
 * during the same loop iteration.
 */
int net_event_loop()
{
	struct epoll_event triggered_events[10];
	int num = epoll_wait(epoll_fd, triggered_events, 10, -1);
	if (num < 0)
		warn(errno, "epoll_wait() falied");
	for (int i = 0; i < num; i++) {
		if (!triggered_events[i].data.ptr) {
			warn(0, "Event with no handler");
			continue;
		}
		handle_t handle = *(handle_t *)(triggered_events[i].data.ptr);
		if (multi_thread) {
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

/* Stops thread pool and closes all opened file descriptors */
int net_deinit()
{
	if (multi_thread)
		finish_pool();
	int ret = close(epoll_fd);
	/* Close clients' sockets */
	for (int i = 0; i < MAX_PEERS; i++) {
		if (peers[i].sock_fd > -1)
			close(peers[i].sock_fd);
	}
	close(sock_fd);
	return 0;
}
