#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <string>

const unsigned int PLAYER_PARAMETERS_NR = 6;
const unsigned int MAX_BUF_SIZE = 32768;

std::string create_request(std::string path, std::string meta);
int connect_with_server(std::string host, std::string path,
                         int servPort, int ourPort, std::string md);
int setup_udp_server(int port);
int process_udp_event(int udp_fd);
bool process_first_tcp_event(int tcp_fd);
void process_normal_tcp_event(int tcp_fd);

void pause_player();
void resume_player();
void send_title();
void finito_amigos();

const std::string METAINT_STR = "icy-metaint:";
const std::string TITLE_STR = "StreamTitle=";

extern int md_int;
extern bool is_md_fetched;
extern std::string last_received_title;

#endif
