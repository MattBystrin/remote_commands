#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "net.h"
#include "thread_pool.h"


#define BUFSIZE 1000
/* Request context */
struct ctx {
	int socket_fd;
	int pipe_fd;
};

void response_handle(void *args)
{
	struct ctx *ctx = args;
	char buf[BUFSIZE];
	int ret = read(ctx->pipe_fd, buf, BUFSIZE);
	ret = write(ctx->socket_fd, buf, BUFSIZE);
}

void request_handle(int fd)
{
}


static bool running = true;

void sigint_handler(int signum)
{
	printf("\nReceived SIGINT. Terminating server\n");
	running = false;
}

int main(int argc, char *argv[])
{
	struct sigaction disp = {
		.sa_handler = sigint_handler
	};
	sigaction(SIGINT, &disp, NULL);

	int sock_fd = net_init("127.0.0.1", 60000);
	printf("Created socket %d\n", sock_fd);
	while (running) {
		net_event_loop();
	}

	net_deinit();
	return 0;
}
