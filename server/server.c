#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <argp.h>
#include <unistd.h>
#include <signal.h>

#include <arpa/inet.h>

#include "net.h"

struct args {
	struct in_addr addr;
	uint16_t port;
	unsigned int threads;
};

static const char doc[] = "Server for remote command execution";

static struct argp_option options[] =
{
  {"addr",   	'a',	"ADDR", 0, 	"Server addres"},
  {"port",   	'p', 	"PORT", 0, 	"Server port"},
  {"threads",   't', 	"NUM",	0, 	"Threads number"},
  {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	static bool addr_prov = false;
	static bool port_prov = false;
	int ret;
	long port;
	struct args *args = state->input;
	switch (key)
	{
	case 'a':
		ret = inet_aton(arg, &args->addr);
		if (!ret)
			argp_error(state, "Error. Not valid address");
		addr_prov = true;
		break;
	case 'p':
		port = strtol(arg, NULL, 10);
		if (errno == EINVAL || errno == ERANGE)
			argp_error(state, "Not valid port");
		if (port > 65535)
			argp_error(state, "Value overflows max port number");
		args->port = port;
		port_prov = true;
		break;
	case 't':
		ret = strtol(arg, NULL, 10);
		if (ret > 16 || ret < 0)
			argp_error(state, "Invalid number of threads provided");
		args->threads = ret;
		break;
	case ARGP_KEY_END:
		if (!(addr_prov && port_prov))
			argp_error(state, "No address or port provided");
		if (state->arg_num > 0) {
			argp_error(state, "Extra arguments provided");
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, NULL, doc};
static bool running = true;

void sigint_handler(int signum)
{
	printf("\nReceived SIGINT. Terminating server\n");
	running = false;
}

int main(int argc, char *argv[])
{
	struct args args = {
		.threads = 0
	};
	argp_parse(&argp, argc, argv, 0, NULL, &args);
	/* Setup signal handler for keyboard interrup in shell */
	struct sigaction disp = {
		.sa_handler = sigint_handler
	};
	sigaction(SIGINT, &disp, NULL);

	net_init(args.addr, args.port, args.threads);

	while (running) {
		net_event_loop();
	}

	net_deinit();
	return 0;
}
