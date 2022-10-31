#include "thread_pool.h"
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <signal.h>

#include "net.h"

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

	net_init("127.0.0.1", 60000, 0);

	while (running) {
		net_event_loop();
	}

	net_deinit();
	return 0;
}
