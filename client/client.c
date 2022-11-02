#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <argp.h>

#include "messages.h"

#define BUF_SIZE 100

struct args {
	char *cmd;
	struct in_addr addr;
	uint16_t port;
};

static const char doc[] = "Client for remote command execution";
static const char arg_doc[] = "COMMAND";

static struct argp_option options[] =
{
  {"addr",   'a', "ADDR", 0, "Server addres"},
  {"port",   'p', "PORT", 0, "Server port"},
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
	case ARGP_KEY_ARG:
		if (state->arg_num > 1) {
			argp_usage(state);
		}
		args->cmd = arg;
		break;
	case ARGP_KEY_END:
		if (!(addr_prov && port_prov))
			argp_error(state, "No address or port provided");
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, arg_doc, doc};


int main(int argc, char *argv[])
{
	struct args args = {
		.cmd = "whoami",
		.addr = 0,
		.port = 0,
	};
	argp_parse(&argp, argc, argv, 0, 0, &args);
	/* Socket creation*/
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
		err(errno, "Socket creation failed");
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(args.port);
	addr.sin_addr = args.addr;
	/* Connect to server */
	int ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret)
		err(errno, "Connect failed");
	/* Prepare payload */
	uint32_t mes_len = strlen(args.cmd) + 1;
	uint32_t hmsl = htonl(mes_len);
	uint8_t *payload = malloc(mes_len + sizeof(mes_len) + 1);
	memcpy(payload, &hmsl, sizeof(mes_len));
	memcpy(payload + sizeof(mes_len), args.cmd, mes_len);
	/* Send */
	int len = send(sock_fd, payload, mes_len + sizeof(mes_len), 0);
	assert(len == mes_len + sizeof(mes_len));
	free(payload);
	/* Receive and print */
	while(1) {
		char buf[BUF_SIZE];
		int len = recv(sock_fd, buf, BUF_SIZE, 0);
		if (len < 0)
			err(errno, "Receive failed");
		if (len == 0)
			break;
		len = write(STDOUT_FILENO, buf, len);
	}
	close(sock_fd);
	return 0;
}
