#include "player.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <string>

// Sets up a udp server working on port 'port'
// or first free port if 'port' is not free.
// Returns file descriptor with newly opened socket, or -1 if failed
int setup_udp_server(int port) {
    struct addrinfo hints;
    struct addrinfo *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or 6, w/e
    hints.ai_socktype = SOCK_DGRAM; // UCP
    hints.ai_flags = AI_PASSIVE; // fill my IP for me

    if ((getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res)) != 0) {
        std::cerr << "Getaddrinfo in setup_udp_server failed" << std::endl;
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "Error in acquiring a socket for udp." << std::endl;
        return -1;
    }

    // find first free port number >= 'port'
    for (int s = -1; s < 0 && port <= MAX_PORT; port++)
        s = bind(sockfd, res->ai_addr, res->ai_addrlen);

    if (port > MAX_PORT) {
        std::cerr << "No port is free to be bound." << std::endl;
        return -1;
    }

    std::cerr << "Found and bound UDP port: " << port << std::endl;
    return sockfd;
}

// Parses and processes further a received UDP message
void process_udp_message(char *buf, int len, int fd, struct sockaddr *client_address) {
    std::cerr << "Received by UDP: " << buf << std::endl;
    if (strncmp(buf, "PAUSE", 5) == 0 && len == 5)
        pause_player();
    else if (strncmp(buf, "PLAY", 4) == 0 && len == 4)
        resume_player();
    else if (strncmp(buf, "TITLE", 5) == 0 && len == 5)
        send_title(fd, *client_address);
    else if (strncmp(buf, "QUIT", 4) == 0 && len == 4)
        finito_amigos();
    else
        std::cerr << "Invalid message type\n";
}

void process_udp_event(int fd) {
    struct sockaddr_in client_address;

    char buf[MAX_BUF_SIZE];
    memset(buf, 0, MAX_BUF_SIZE);
    int flags = 0; // nothing special
    socklen_t rcva_len = (socklen_t) sizeof(client_address);
    int len = recvfrom(fd, buf, sizeof buf, flags,
            (struct sockaddr *) &client_address, &rcva_len);
    if (len < 0)
        std::cerr << "Recvfrom error, error code: " << errno << std::endl;
    else
        process_udp_message(buf, len, fd, (struct sockaddr *) &client_address);
}
