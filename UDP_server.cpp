#include "player.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <string>

int setup_udp_server(int port) {
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or 6, w/e
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // fill my IP for me

    // find first free port number >= "port"
    for (;;) {
        getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res);

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) {
            freeaddrinfo(res);
            std::cerr << "socket error" << std::endl;
            return -1;
        }

        if ((bind(sockfd, res->ai_addr, res->ai_addrlen)) >= 0)
            break;
        else
            port++;
    }
    return sockfd;
}

void process_udp_message(char *buf) {
    if (buf == "PAUSE\0")
        pause_player();
    else if (buf == "PLAY\0")
        resume_player();
    else if (buf == "TITLE\0")
        send_title();
    else if (buf == "QUIT\0") {
        printf("quit\n");
        finito_amigos();
    }
    else {
        
    }
}

int process_udp_event(int fd) {
    struct sockaddr_in client_address;

    char buf[MAX_BUF_SIZE];
    int flags = 0; // nothing special
    socklen_t rcva_len = (socklen_t) sizeof(client_address);
    int len = recvfrom(fd, buf, sizeof buf, flags,
            (struct sockaddr *) &client_address, &rcva_len);
    if (len < 0) {
        std::cerr << "recvfrom error" << std::endl;
        return 1;
    }
    process_udp_message(buf);
}
