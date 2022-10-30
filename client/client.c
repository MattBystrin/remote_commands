#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

int main(int argc, char *argv[])
{
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(60000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
	if (argc < 2) {
		printf("Not enought args\n");
		return -1;
	}
	char *mes = argv[1];
	uint32_t mes_len = strlen(mes) + 1;
	uint32_t hmsl = htonl(mes_len);

	uint8_t *payload = malloc(mes_len + sizeof(mes_len) + 1);

	memcpy(payload, &hmsl, sizeof(mes_len));
	memcpy(payload + sizeof(mes_len), mes, mes_len);

	send(sock_fd, payload, mes_len + sizeof(mes_len), 0);
	free(payload);
	while(1) {
		char buf[100];
		int len = recv(sock_fd, buf, 100, 0);
		if (len == 0)
			break;
		len = write(STDOUT_FILENO, buf, len);
	}
	close(sock_fd);
	return 0;
}
