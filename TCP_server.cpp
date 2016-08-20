#include <iostream>
#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

const unsigned int BUFFER_SIZE = 2000;

/* create initial request in ICY protocol */
std::string create_request(std::string path, std::string meta) {
    return "GET " + path + " HTTP/1.0\r\n" +
           "Icy-MetaData:" + meta + "\r\n" +
           "\r\n";
}

int connect_with_server(std::string host, std::string path,
        int servPort, int ourPort, std::string md) {
	struct addrinfo hints, *res;
	int sockfd;
    int s;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;      // either ip4 or 6
	hints.ai_socktype = SOCK_STREAM;  // tcp
    // TODO: sprawdzic czy warto/trzeba dodac AI_PASSIVE w hints.flags?

    // get address
	if ((s = getaddrinfo(host.c_str(), std::to_string(servPort).c_str(), &hints, &res)) != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(s) << std::endl;
        return 1;
	}

	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "socket error" << std::endl;
        return 1;
    }

	// connect
	s = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (s < 0) {
        freeaddrinfo(res);
        std::cerr << "connect error" << std::endl;
        return 1;
    }
    freeaddrinfo(res);
    
    std::string request = create_request(path, (md == "yes") ? "1" : "0");
    printf("%s\n", request.c_str());

    // send request to server
    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "send error" << std::endl;
        return 1;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // receive the answer
    s = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);

    // powinno dzialac
    printf("%s\nGG\n", buffer);
}
