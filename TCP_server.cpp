#include "player.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // "close"

/* create initial request in ICY protocol */
std::string create_request(std::string path, std::string meta) {
    return "GET " + path + " HTTP/1.0\r\n" +
           "Icy-MetaData:" + meta + "\r\n" +
           "\r\n";
}

// true if ok (there was metadata, and its parsed), false otherwise
bool parse_the_metadata(char *buf, int len) {
    std::string s (buf, len);
    if (!is_md_fetched) {
        // no md_interval known yet
        size_t pos = s.find(METAINT_STR);
        if (pos != std::string::npos) {
            std::cerr << "packet with metadata:" << std::endl << buf << std::endl;
            md_int = 0;
            pos += METAINT_STR.size();
            while (std::isdigit(buf[pos])) {
                // lets parse the md_int (global variable)
                md_int = md_int * 10 + (buf[pos] - '0');
                pos++;
            }
            is_md_fetched = true;
            std::cerr << "md_int = " << md_int << std::endl;
        }
    }
    size_t pos = s.find(TITLE_STR);
    if (pos != std::string::npos) {
        std::cerr << "JEST! jak duzy? " << len << std::endl;
        std::cerr << buf << std::endl;
        pos += TITLE_STR.size();
        last_received_title = "";
        while (buf[pos] != ';') {
            last_received_title += buf[pos];
            pos++;
        }
        std::cerr << "TITLE: " << last_received_title << std::endl;
        return true;
    }
    return false;
}

int connect_with_server(std::string host, std::string path,
                        int servPort, std::string md) {
	struct addrinfo hints, *res;
	int sockfd;
    int s;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;      // either ip4 or 6
	hints.ai_socktype = SOCK_STREAM;  // tcp
    // TODO: sprawdzic czy warto/trzeba dodac AI_PASSIVE w hints.flags?

	if ((s = getaddrinfo(host.c_str(), std::to_string(servPort).c_str(), &hints, &res)) != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(s) << std::endl;
        return -1;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "socket error" << std::endl;
        return -1;
    }

	s = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (s < 0) {
        freeaddrinfo(res);
        close(sockfd);
        std::cerr << "connect error" << std::endl;
        return -1;
    }
    freeaddrinfo(res);
    
    std::string request = create_request(path, (md == "yes") ? "1" : "0");
    std::cerr << "request to server:\n" << request << std::endl;

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        close(sockfd);
        std::cerr << "send error" << std::endl;
        return -1;
    }

    char buffer[MAX_BUF_SIZE];
    memset(buffer, 0, MAX_BUF_SIZE);

    // receive the answer
    s = recv(sockfd, buffer, MAX_BUF_SIZE, 0);
    std::cerr << "initial response from server:\n" << buffer << std::endl;

    return sockfd;
}

bool process_first_tcp_event(int fd, bool is_player_paused) {
    char buf[MAX_BUF_SIZE];
    memset(buf, 0, MAX_BUF_SIZE);

    const int MAX_INTERVAL = 65336;
    bool sizes[MAX_INTERVAL];
    int sizes_size = std::min(MAX_INTERVAL, md_int);
    std::fill(sizes, sizes + sizes_size, true);

    int len = recv(fd, buf, MAX_BUF_SIZE, 0);

    static int pom = 0;
    if (!is_player_paused) {
        bool is_titled = parse_the_metadata(buf, len);
        pom += len;
        //std::cerr << "len: " << len << "       lenmod: " << len % md_int << " is_tilted " << is_titled << std::endl;
        // std::cout.write(buf, len);
        return is_titled;
    }
    return false;
}

void process_normal_tcp_event(int fd) {
    int BUFFER_SIZE = 20000;
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);

    int len = recv(fd, buf, BUFFER_SIZE, 0);

    static int till_metadata = md_int;
    std::cerr << till_metadata << std::endl;
    if (till_metadata == 0) {
        parse_the_metadata(buf, len);
        till_metadata = md_int;
    }
    else if (till_metadata > 0) {
        std::cout.write(buf, len);
        till_metadata -= len;
    }
}
