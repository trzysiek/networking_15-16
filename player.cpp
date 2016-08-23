#include "player.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#define TCP_S 0
#define UDP_S 1

// GLOBAL RESOURCES
const unsigned int SOCKETS_NR = 2;
struct pollfd fds[SOCKETS_NR];
int backup_udp_fd;
int backup_tcp_fd;
int md_int = 8192;  // metadata interval (default 8192)
std::string last_received_title;  // current title of song
bool is_md_fetched; // says if we already received metadata and set it to
                    // correct value instead of default

int run_main_player(int tcp_fd, int udp_fd) {
    std::cerr << "main player is being run. tcp_fd: " << tcp_fd
              << " udp_fd: " << udp_fd << std::endl;

    backup_udp_fd = udp_fd;
    backup_tcp_fd = tcp_fd;

    char buf[MAX_BUF_SIZE];

    fds[TCP_S].fd = tcp_fd;
    fds[TCP_S].events = POLLIN;

    fds[UDP_S].fd = udp_fd;
    fds[UDP_S].events = POLLIN;

    bool is_first_tcp = true;

    for (;;) {
        fds[0].revents = 0;
        fds[1].revents = 0;

        if ((poll(fds, 2, -1)) == -1) {
            std::cerr << "error in poll\n";
            return 1;
        }
        if (fds[0].revents & POLLIN) {
            //fix
            //printf("0 ");
                if (process_first_tcp_event(tcp_fd)) {}
        }
        if (fds[1].revents & POLLIN) {
            std::cerr << "1\n";
            process_udp_event(udp_fd);
        }
    }
}

void pause_player() {
    fds[TCP_S].fd = -1;
}

void resume_player() {
    fds[TCP_S].fd = backup_tcp_fd;
}

void send_title() {
}

void finito_amigos() {
    close(backup_tcp_fd);
    close(backup_udp_fd);
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
            int tcp_fd = connect_with_server(host, path, servPort, ourPort, md);
            int udp_fd = setup_udp_server(ourPort); 
            run_main_player(tcp_fd, udp_fd);
            finito_amigos();
        }
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }    
    return 0;
}
