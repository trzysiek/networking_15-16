#include "player.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <string>

// todo fix jak nie bedzie zadnego portu wolnego
int setup_udp_server(int port) {
    struct addrinfo hints;
    struct addrinfo *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or 6, w/e
    hints.ai_socktype = SOCK_DGRAM; // UCP
    hints.ai_flags = AI_PASSIVE; // fill my IP for me

    if ((getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res)) != 0) {
        std::cerr << "getaddrinfo in setup_udp_server failed\n";
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "socket error" << std::endl;
        return -1;
    }

    // find first free port number >= "port"
    do {
        std::cerr << "port: " << port << std::endl;
        if ((bind(sockfd, res->ai_addr, res->ai_addrlen)) >= 0)
            break;
        else
            port++;
    } while (port <= MAX_BUF_SIZE); // TODO poprawic :D
    std::cerr << "UDP port: " << port << std::endl;
    struct sockaddr_in client_address;

    char buf[MAX_BUF_SIZE];
    int flags = 0; // nothing special
    socklen_t rcva_len = (socklen_t) sizeof(client_address);
    int len = recvfrom(sockfd, buf, sizeof buf, flags,
            (struct sockaddr *) &client_address, &rcva_len);
    if (len < 0) {
        std::cerr << "a recvfrom error, error code: " << errno << std::endl;
        return 1;
    }
    return sockfd;
}

void process_udp_message(char *buf) {
    std::cerr << "udp buf\n" << buf << std::endl;
    if (buf == "PAUSE\0")
        pause_player();
    else if (buf == "PLAY\0")
        resume_player();
    else if (buf == "TITLE\0")
        send_title();
    else if (buf == "QUIT\0") {
        std::cerr << "Received QUIT message\n";
        finito_amigos();
    }
    else
        std::cerr << "Invalid message type\n";
}

int process_udp_event(int fd) {
    struct sockaddr_in client_address;

    char buf[MAX_BUF_SIZE];
    memset(buf, 0, MAX_BUF_SIZE);
    int flags = 0; // nothing special
    socklen_t rcva_len = (socklen_t) sizeof(client_address);
    int len = recvfrom(fd, buf, sizeof buf, flags,
            (struct sockaddr *) &client_address, &rcva_len);
    if (len < 0) {
        std::cerr << "recvfrom error, error code: " << errno << std::endl;
        return 1;
    }
    process_udp_message(buf);
}
