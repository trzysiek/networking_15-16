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

// erases not needed header data
std::string eliminate_initial_response_header(std::string msg) {
    const std::regex header {"ICY.*(\\n|\\r\\n)icy-notice1:.*(\\n|\\r\\n)icy-notice2:.*(\\n|\\r\\n)"};
    const std::string replacer = "";
    return regex_replace(msg, header, replacer);
}

std::string eliminate_metadata(std::string msg) {
    const std::regex metadata {"icy-name:.*(?:\\n|\\r\\n)(.*(?:\\n|\\r\\n))*icy-br:\\d*(\\n|\\r\\n)(\\n|\\r\\n)"};
    const std::string replacer = "";
    return regex_replace(msg, metadata, replacer);
}

bool parse_the_metaint(std::string msg) {
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
bool parse_the_title(std::string s) {
    size_t pos = s.find(TITLE_STR);
    if (pos != std::string::npos) {
        pos += TITLE_STR.size();
        last_received_title = "";
        while (s[pos] != ';') {
            last_received_title += s[pos];
            pos++;
        }
        std::cerr << "TITLE: " << last_received_title << std::endl;
        return true;
    }
    return false;
}

void parse_tcp_message(std::string msg) {
    static int byte_count = 0; // counts bytes till next metadata
    if (!is_md_int_fetched)
        if (parse_the_metaint(msg))
            is_md_int_fetched = true;

    // TODO w metadacie mb?
    if (parse_the_title(msg))
        std::cerr << "TITLED!\n";

    std::string msg2 = eliminate_initial_response_header(msg);
    std::string msg3 = eliminate_metadata(msg2);
    std::string final_msg = "";

    std::cerr << byte_count << " " << msg3.size() << " " << msg.size() << std::endl;

    if (byte_count >= md_int) {
        // we should have received meta-data
        int md_len = int(msg[0]) * 16 + 1;
        for (int i = md_len; i < (int)msg3.size(); ++i)
            final_msg += msg3[i];
        byte_count -= md_int;

        std::cerr << "\nGG " << int(msg[0]) << " " << msg.size() << " " << final_msg.size() << " " << byte_count << std::endl;

    }
    else if (byte_count + (int)msg3.size() <= md_int)
        final_msg = msg3;
    else {
        // we cut metadata in the middle
        int it = 0;
        while (byte_count < md_int) {
            final_msg += msg3[it];
            byte_count++;
            it++;
        }
        parse_tcp_message(msg3.substr(it, MAX_BUF_SIZE));
    }

    byte_count += final_msg.size();

    if (is_output_to_file)
        output_to_file_stream << final_msg;
    else
        write(STDOUT_FILENO, final_msg.c_str(), final_msg.size());
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
    parse_tcp_message(std::string(buf, s));
    // sprawdzac -1 czy cos?

    return sockfd;
}

bool process_tcp_event(int fd, bool is_player_paused) {
    char buf[MAX_BUF_SIZE];
    memset(buf, 0, MAX_BUF_SIZE);

    int len = recv(fd, buf, MAX_BUF_SIZE, 0);

    parse_tcp_message(std::string(buf, len));
    return true;
}
