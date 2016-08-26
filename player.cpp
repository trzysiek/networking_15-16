#include "player.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

const int TCP_S = 0;
const int UDP_S = 1;

// GLOBAL RESOURCES
struct pollfd fds[PLAYER_SOCKETS_NR];
int backup_udp_fd;
int backup_tcp_fd;
int md_int = 8192;  // metadata interval (default 8192)
std::string last_received_title;  // current title of song
bool is_md_in_data;
bool is_md_int_fetched; // says if we already received metadata and set it to
                    // correct value instead of default
bool is_player_closed;
bool is_player_paused;
bool is_output_to_file;
std::ofstream output_to_file_stream;

int run_main_player(int tcp_fd, int udp_fd) {
    std::cerr << "Main player is being run." << std::endl;

    char buf[MAX_BUF_SIZE];

    // setting global variables
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

        if ((poll(fds, 2, -1)) == -1) {
            std::cerr << "Error in poll." << std::endl;
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
    if (is_output_to_file)
        output_to_file_stream.close();
    std::cerr << "Player closed." << std::endl;
}

Parameters parse_parameters(int argc, char* argv[]) {
    Parameters p;
    std::string str_md;

    po::options_description desc("OPTIONS");
    desc.add_options()
        ("host", po::value<std::string>(&p.host), "host server name")
        ("path", po::value<std::string>(&p.path), "path to resource, commonly just /")
        ("r-port", po::value<int>(&p.serv_port), "server's port")
        ("file", po::value<std::string>(&p.output_file), "path to file, to which audio is saved"
              ", or '-' when audio on stdout")
        ("m-port", po::value<int>(&p.our_udp_port), "UDP port on which we listen")
        ("md", po::value<std::string>(&str_md), "'yes' if metadata on, 'no' otherwise")
    ;

    po::positional_options_description pod;
    pod.add("host", 1);
    pod.add("path", 1);
    pod.add("r-port", 1); // TODO regexem sprawdzac czy ok port?
    pod.add("file", 1);
    pod.add("m-port", 1);
    pod.add("md", 1);
    
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(pod).run(), vm);
    po::notify(vm);

    if (vm.size() != PLAYER_PARAMETERS_NR) {
        std::cerr << "Usage: player OPTIONS\n";
        std::cerr << desc;
        throw new std::runtime_error("Unmatched number of player parameters."); 
    }

    if (str_md == "yes")
        p.md = true;
    else if (str_md == "no")
        p.md = false;
    else
        throw new std::runtime_error("Metadata must be either 'yes' or 'no' (without '').\n");

    return p;
}

int main(int argc, char* argv[]) {
    Parameters p;
    try {
        p = parse_parameters(argc, argv);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }    

    std::cerr << "Parameters for player:" << std::endl
              << "host: " << p.host << std::endl
              << "path: " << p.path << std::endl
              << "serv port: " << p.serv_port << std::endl
              << "output file: " << p.output_file << std::endl
              << "our UDP port: " << p.our_udp_port << std::endl
              << "metadata? " << p.md << std::endl << std::endl;

    if (p.output_file != "-") {
        is_output_to_file = true;
        output_to_file_stream.open(p.output_file, std::ofstream::binary);
    }
    else
        is_output_to_file = false;

    int tcp_fd = setup_tcp_client(p.host, p.path, p.serv_port, p.md);
    int udp_fd = setup_udp_server(p.our_udp_port); 
    run_main_player(tcp_fd, udp_fd);

    return 0;
}
