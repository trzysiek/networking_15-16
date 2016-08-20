#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <string>

const unsigned int PLAYER_PARAMETERS_NR = 6;

std::string create_request(std::string path, std::string meta);
void connect_with_server(std::string host, std::string path,
                         int servPort, int ourPort, std::string md);

#endif
