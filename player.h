#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <string>
#include <fstream>

const unsigned int PLAYER_SOCKETS_NR = 2;
const unsigned int PLAYER_PARAMETERS_NR = 6;
const unsigned int MAX_BUF_SIZE = 32768;
const int MAX_PORT = 65335;

const std::string TITLE_STR = "StreamTitle=";

struct Parameters {
    std::string host;
    std::string path;
    int serv_port;
    int our_udp_port;
    std::string output_file;
    bool md;
};

std::string create_request(std::string path, std::string meta);
int setup_tcp_client(std::string host, std::string path,
                     int servPort, bool md);
int setup_udp_server(int port);
void process_udp_event(int udp_fd);
bool process_tcp_event(int tcp_fd, bool is_player_paused);

void pause_player();
void resume_player();
void send_title(int udp_fd, struct sockaddr client_address);
void finito_amigos();

extern int md_int;
extern std::string last_received_title;
extern bool is_md_in_data;
extern bool is_md_int_fetched;
extern bool is_player_paused;
extern bool is_output_to_file;
extern std::ofstream output_to_file_stream;

#endif
