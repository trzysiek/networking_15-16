#include "player.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // "close"

const unsigned int BUFFER_SIZE = 2000;

/* create initial request in ICY protocol */
std::string create_request(std::string path, std::string meta) {
    return "GET " + path + " HTTP/1.0\r\n" +
           "Icy-MetaData:" + meta + "\r\n" +
           "\r\n";
}

// true if ok, false otherwise
bool parse_the_metadata(char *buf, int len) {
    std::string s (buf, len);
    if (!fetched_md) {
        // no md_interval known yet
        size_t pos = s.find(METAINT_STR);
        std::cout << buf << std::endl;
        if (pos != std::string::npos) {
            // lets parse the md_int (global)
            md_int = 0;
            pos += METAINT_STR.size();
            while (std::isdigit(buf[pos])) {
                md_int = md_int * 10 + (buf[pos] - '0');
                pos++;
            }
            fetched_md = true;
        }
    }
    size_t pos = s.find(TITLE_STR);
    if (pos != std::string::npos) {
        std::cout << "JEST!\n";
        std::cout << buf << std::endl;
        pos += TITLE_STR.size();
        title = "";
        while (buf[pos] != ';') {
            title += buf[pos];
            pos++;
        }
        std::cout << "TITLE: " << title << std::endl;
        return true;
    }
    return false;
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
        return -1;
	}

	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "socket error" << std::endl;
        return -1;
    }

	// connect
	s = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (s < 0) {
        freeaddrinfo(res);
        close(sockfd);
        std::cerr << "connect error" << std::endl;
        return -1;
    }
    freeaddrinfo(res);
    
    std::string request = create_request(path, (md == "yes") ? "1" : "0");
    printf("%s\n", request.c_str());

    // send request to server
    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        close(sockfd);
        std::cerr << "send error" << std::endl;
        return -1;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // receive the answer
    s = recv(sockfd, buffer, BUFFER_SIZE, 0);
    printf("len1: %d\n", s);

    // powinno dzialac
    printf("%s\nGG\n", buffer);

    return sockfd;
}

bool process_first_tcp_event(int fd) {
    int BUFFER_SIZE = 20000;
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);

    int MAX_INTERVAL = 65336;
    bool sizes[MAX_INTERVAL];
    int sizes_size = std::min(MAX_INTERVAL, md_int);
    std::fill(sizes, sizes + sizes_size, true);

    int len = recv(fd, buf, BUFFER_SIZE, 0);

    static int pom = 0;
    bool is_titled = parse_the_metadata(buf, len);
    pom += len;
    printf("len: %d      %d titled? %d\n", len, pom % 8192, is_titled);
    //std::cout.write(buf, len); std::cout << "\n\n";}

    return is_titled;
}

void process_normal_tcp_event(int fd) {
    int BUFFER_SIZE = 20000;
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);

    int len = recv(fd, buf, BUFFER_SIZE, 0);

    static int till_metadata = md_int;
    if (till_metadata == 0) {
        parse_the_metadata(buf, len);
        till_metadata = md_int;
    }
    else {
        std::cout.write(buf, len);
        till_metadata -= len;
    }
}
