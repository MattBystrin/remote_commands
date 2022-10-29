#include <stdio.h>
#include <unistd.h>
#include "thread_pool.h"

int main(int argc, char *argv[])
{
	allocate_pool(4);
	push_task();
	finish_pool();
	return 0;
}

	/* if (argc < 3)
		return -1;
	FILE *pipe = popen(argv[1], "r");
	FILE *pipe2 = popen(argv[2], "r");
	int pipe_fd = pipe->_fileno;
	int pipe2_fd = pipe2->_fileno;
	char buf[BUFSIZE];

	int epoll_fd = epoll_create(1);
	if (epoll_fd == -1) {
		printf("Error creating epoll\n");
		return -1;
	}

	struct epoll_event ev[2];
	ev[0].events = EPOLLIN;
	ev[0].data.fd = pipe_fd;
	ev[1].events = EPOLLIN;
	ev[1].data.fd = pipe2_fd;

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fd, ev);
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe2_fd, ev+1);

	struct epoll_event events[5];
	int evnum;

	while(1) {
		evnum = epoll_wait(epoll_fd, events, 5, 0);
		if (evnum == -1) {
			printf("Epoll error\n");
			break;
		}
		for (int i = 0; i < evnum; i++) {
			int ret = read(events[i].data.fd, buf, BUFSIZE);
			write(STDOUT_FILENO, buf, ret);
		}
	}

	if (close(epoll_fd)) {
		printf("Error closing epoll descriptor\n");
		return -1;
	}

	pclose(pipe); */
