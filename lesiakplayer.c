#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define SERVER 0
#define MASTER 1
#define REQUEST_MAX_LENGTH 1000

void test() {
	printf("DOTARLEM TU");
	fflush(stdout);
}

//Weryfikacja poprawności parametrów programu
int verifyInput(char *par[]) {
	if (!strcmp(par[6], "no") && !strcmp(par[6], "yes")) {
		return 0;
	}

	return 1;
}

void createRequest(char buffer[], char *path, int meta) {
	memset(buffer, 0, REQUEST_MAX_LENGTH);
	sprintf(buffer, "GET %s HTTP/1.0\r\nIcy-MetaData:%d\r\n\r\n", path, meta);
}

int main(int argc, char *argv[]) {
	FILE *fp;
	int PAUSE = FALSE;
	int m_port, r_port;
	int md = 0;
	int meta_int = 0;
	int ret;
	char *host, *path, *file_name;
	struct pollfd sockets[2];
	struct addrinfo addr_hints, *addr_result;
	char request[REQUEST_MAX_LENGTH];
	char response[REQUEST_MAX_LENGTH];

	//Wczytanie parametrów programu
	if (argc != 7) {
		fputs("Wrong parameters\n", stderr);
		return 1;
	} else if(verifyInput(argv)) {
		//TODO: Moze by tak usunac to w sumie?
		host = argv[1];
		path = argv[2];
		r_port = atoi(argv[3]);
		file_name = argv[4];
		m_port = atoi(argv[5]);
		if (!strcmp(argv[6], "yes"))
			md = 1;
	} else {
		return 1;
	}

	fp = fopen(file_name, "w");

	//Przygotowanie struktury pollfd
	sockets[SERVER].events = POLLIN;
	sockets[SERVER].revents = 0;
	sockets[MASTER].events = POLLIN;
	sockets[MASTER].revents = 0;
	memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_flags = 0;
	addr_hints.ai_family = AF_UNSPEC;
	addr_hints.ai_socktype = SOCK_STREAM;
	addr_hints.ai_protocol = IPPROTO_TCP;
	//TODO: dopisać path do hosta chyba
	if (getaddrinfo(host, argv[3], &addr_hints, &addr_result) != 0) {
		fputs("Getaddrinfo failure\n", stderr);
		return 1;
	}
	sockets[SERVER].fd = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
	if (sockets[SERVER].fd < 0) {
		fputs("Socket failure\n", stderr);
		return 1;
	}

	//Łączenie z serwerem radia
	if (connect(sockets[SERVER].fd, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
		fputs("Connection failure\n", stderr);
		return 1;
	}

	createRequest(request, path, md);
	//Wysłanie zapytania do serwera
	if (send(sockets[SERVER].fd, request, strlen(request), 0) < 0)
		fputs("Send failure", stderr);

	//TODO: Obsługa mastera
	sockets[MASTER].fd = -1;

	//Główna pętla programu
	do {
		memset(response, 0, REQUEST_MAX_LENGTH);

		sockets[SERVER].revents = 0;
		sockets[MASTER].revents = 0;

		ret = poll(sockets, 2, 0);

		if (ret < 0) {
			fputs("Poll failure", stderr);
			return 1;
		} else {
			if (PAUSE == FALSE && (sockets[SERVER].revents & POLLIN)) {
				read(sockets[SERVER].fd, response, REQUEST_MAX_LENGTH);
			}
		}
		//printf("%s", response);
		fprintf(fp, "%s", response);

	} while(1);
}
