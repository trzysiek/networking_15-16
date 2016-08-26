#include "player.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <regex>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // "close"

// create initial request in ICY protocol
std::string create_request(std::string path, bool md) {
    return "GET " + path + " HTTP/1.0\r\n" +
           "Icy-MetaData:" + (md ? '1' : '0') + "\r\n" +
           "\r\n";
}

bool parse_the_metaint(char *buf, int len) {
    std::string msg(buf, len);
    const std::regex reg_ex {"icy-metaint:([1-9]\\d*)"};
    std::smatch matches;

    if (regex_search(msg, matches, reg_ex)) {
        std::string md_int_as_str = std::string(matches[1].first, matches[2].second);
        md_int = std::stoi(md_int_as_str);
        std::cerr << "Received md_int is: " << md_int << std::endl;
        return true;
    }
    return false;
}

// true if ok (there was metadata, and its parsed), false otherwise
bool parse_the_metadata(char *buf, int len) {
    std::string s (buf, len);
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

int setup_tcp_client(std::string host, std::string path,
                     int servPort, bool md) {
	struct addrinfo hints, *res;
	int sockfd;
    int s;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;      // either ip4 or 6
	hints.ai_socktype = SOCK_STREAM;  // tcp
    // TODO: sprawdzic czy warto/trzeba dodac AI_PASSIVE w hints.flags?

	if ((s = getaddrinfo(host.c_str(), std::to_string(servPort).c_str(), &hints, &res)) != 0) {
		std::cerr << "Getaddrinfo error in TCP client: " << gai_strerror(s) << std::endl;
        return -1;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "Socket error during TCP client setup." << std::endl;
        return -1;
    }

	s = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (s < 0) {
        freeaddrinfo(res);
        close(sockfd);
        std::cerr << "Connect error during TCP client setup." << std::endl;
        return -1;
    }
    freeaddrinfo(res);
    
    std::string request = create_request(path, md);
    std::cerr << "Request to server:\n" << request << std::endl;

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        close(sockfd);
        std::cerr << "Send error in TCP client setup." << std::endl;
        return -1;
    }

    char buf[MAX_BUF_SIZE];
    memset(buf, 0, MAX_BUF_SIZE);

    s = recv(sockfd, buf, MAX_BUF_SIZE, 0);
    std::cerr << "initial response from server:\n" << buf << std::endl;

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

    std::cerr << "dlugosc 1. pakietu to: " << len << ", a pierwszy bit to " << int(buf[0]) << std::endl;

    if (!is_md_int_fetched)
        if (parse_the_metaint(buf, len))
            is_md_int_fetched = true;
    if (is_md_in_data) {
        
    }

    //static int pom = 0;
    //pom++;
    //pom += len;
    //std::cerr << "len: " << len << "       lenmod: " << len % md_int << std::endl;
    if (is_output_to_file)
        output_to_file_stream.write(buf, len);
    else
        std::cout.write(buf, len);
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
