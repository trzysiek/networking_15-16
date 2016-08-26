#include "player.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>

const int TCP_S = 0;
const int UDP_S = 1;

// GLOBAL RESOURCES
struct pollfd fds[PLAYER_SOCKETS_NR];
int backup_udp_fd;
int backup_tcp_fd;
int md_int = 8192;  // metadata interval (default 8192)
std::string last_received_title;  // current title of song
bool is_md_fetched; // says if we already received metadata and set it to
                    // correct value instead of default
bool is_player_closed;
bool is_player_paused;

int run_main_player(int tcp_fd, int udp_fd) {
    std::cerr << "Main player is being run." << std::endl;

    char buf[MAX_BUF_SIZE];

    // set global variables
    backup_udp_fd = udp_fd;
    backup_tcp_fd = tcp_fd;

    fds[TCP_S].fd = tcp_fd;
    fds[TCP_S].events = POLLIN;

    fds[UDP_S].fd = udp_fd;
    fds[UDP_S].events = POLLIN | POLLOUT;

    bool is_first_tcp = true;

    while (!is_player_closed) {
        fds[TCP_S].revents = 0;
        fds[UDP_S].revents = 0;

        fds[TCP_S].fd = -1;

        if ((poll(fds, 2, -1)) == -1) {
            std::cerr << "error in poll\n";
            return 1;
        }
        if (fds[TCP_S].revents & POLLIN) {
            if (process_first_tcp_event(tcp_fd, is_player_paused)) {}
        }
        if (fds[UDP_S].revents & POLLIN) {
            process_udp_event(udp_fd);
        }
    }
}

void pause_player() {
    is_player_paused = true;
    std::cerr << "Player paused." << std::endl;
}

void resume_player() {
    is_player_paused = false;
    std::cerr << "Player resumed." << std::endl;
}

void send_title(int udp_fd, struct sockaddr client_address) {
    socklen_t snda_len = (socklen_t) sizeof(client_address);
    int flags = 0;
    int sent_len = sendto(udp_fd, last_received_title.c_str(),
            last_received_title.size(), flags, &client_address, snda_len);
    if (sent_len != last_received_title.size())
        std::cerr << "Error in sending title." << std::endl;
    else
        std::cerr << "Title sent, title: " << last_received_title << std::endl;
}

void finito_amigos() {
    close(backup_tcp_fd);
    close(backup_udp_fd);
    std::cerr << "Player closed." << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host;
    std::string path;
    int servPort;
    int ourPort;
    std::string outputFile;
    std::string md;

    try {
        po::options_description desc("OPTIONS");
        desc.add_options()
            ("host", po::value<std::string>(&host), "host server name")
            ("path", po::value<std::string>(&path), "path to resource, commonly just /")
            ("r-port", po::value<int>(&servPort), "server's port")
            ("file", po::value<std::string>(&outputFile), "path to file, to which audio is saved"
                  ", or '-' when audio on stdout")
            ("m-port", po::value<int>(&ourPort), "UDP port on which we listen")
            ("md", po::value<std::string>(&md), "'yes' if metadata on, 'no' otherwise")
        ;

        po::positional_options_description p;
        p.add("host", 1);
        p.add("path", 1);
        p.add("r-port", 1);
        p.add("file", 1);
        p.add("m-port", 1);
        p.add("md", 1);
        
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);
    
        if (vm.size() != PLAYER_PARAMETERS_NR) {
            std::cout << "Usage: player OPTIONS\n";
            std::cout << desc;
            return 1;
        }
        else {
            std::cerr << "parameters for player:" << std::endl
                      << host << std::endl
                      << path << std::endl
                      << servPort << std::endl
                      << outputFile << std::endl
                      << ourPort << std::endl
                      << md << std::endl;

            // dodac jakies sprawdzenia czy md, porty poprawne, itp.
            int tcp_fd = connect_with_server(host, path, servPort, md);
            int udp_fd = setup_udp_server(ourPort); 
            run_main_player(tcp_fd, udp_fd);
        }
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }    
    return 0;
}
