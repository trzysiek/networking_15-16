#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <limits.h>
#include <stack>
#include <set>
#include <iostream>
#include <regex>

using namespace std;

const int MAX_CONNECTIONS = 1000;
const int BUFF_SIZE = 100;

struct pollfd connections[MAX_CONNECTIONS];
char player_replies[MAX_CONNECTIONS][BUFF_SIZE];
stack<int> available_id;
set<int> player_id;

void process_telnet_input(char buff[], int index) {
	regex start{"^START (?:([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) ((?:(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) (?:\\w*\\/)+ [1-9]\\d* (?:[\\w,\\s-]+\\.[A-Za-z3]{3}) ([1-9]\\d*) (?:no|yes))(?:\\r\\n|\\r|\\n)$"};
	regex player_command{"^(PAUSE|PLAY|TITLE|QUIT) ([1-9]\\d*)(\\r\\n|\\r|\\n)$"};
	regex at{"^AT ((?:[0-9]|0[0-9]|1[0-9]|2[0-3])\\.[0-5][0-9]) ([1-9]\\d*) (?:([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) ((?:(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) (?:\\w*\\/)+ [1-9]\\d* (?:[\\w,\\s-]+\\.[A-Za-z3]{3}) ([1-9]\\d*) (?:no|yes))(?:\\r\\n|\\r|\\n)$"};
	smatch match;
	string input(buff);
	int id;

	if (regex_search(input, match, start)) {
		//Otwórz program player na odpowiednim hoście przez ssh
		string cmd = "ssh " + string(match[1].first, match[1].second) + " ./player " + string(match[2].first , match[2].second);
		system(cmd.c_str());
		//Zarezerwuj id dla nowego playera
		id = available_id.top();
		available_id.pop();
		player_id.insert(id);
		//Stwórz UDP socketa dla nowego playera
		struct addrinfo addr_hints;
  		struct addrinfo *addr_result;
  		memset(&addr_hints, 0, sizeof addr_hints);
		addr_hints.ai_family = AF_UNSPEC;
		addr_hints.ai_socktype = SOCK_DGRAM;
		addr_hints.ai_flags = AI_PASSIVE;
		getaddrinfo(string(match[1].first, match[1].second).c_str(), string(match[3].first, match[3].second).c_str(), &addr_hints, &addr_result);
		int sockfd = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
		if (sockfd < 0) {
			cerr << "Socker error" << endl;
		}
		connect(sockfd, addr_result->ai_addr, addr_result->ai_protocol);
		connections[id].fd = sockfd;

		//TODO:Wpisać player_replies i dać POLLOUT na sesje telnetowa o indeksie index????
		
	} else if (regex_search(input, match, player_command)) {
		id = atoi(string(match[2].first, match[2].second).c_str());
		string message = string(match[1].first, match[1].second);
		send(connections[id].fd, &message, message.length(), 0);
		//TODO:1.Wpisać player_replies i dać POLLOUT JEŚLI TO NIE TITLE 2.Jeśli title to jak otrzymamy tytuł damy POLLOUT itd.

	} else if (regex_search(input, match, at)) {

	} else {
		cerr << "Wrong command received from telnet session" << endl;
	}
}

//Zwróć port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

/*

TODO: 	1. FREEADDRINFO, CLOSE SOCKETY
	2. SPRAWDZENIE PARAMETRÓW PROGRAMU LEPSZE
	3. WYPISYWAC ERRORA JAK SSH PRÓBUJE PYTAĆ O HASŁO
	4. ERRORY W FUNKCJACH INNYCH NIZ MAIN TEZ POWINNY WYWALAC PROGRAM
	5. ERRORY WYPISYWAĆ TELNETOWI
	6. REGEX_REPLACE żeby wyjebać bajty kontrolne z telneta
	7. IGNOROWANIE KOMUNIKATÓW DLA AT PLAYERÓW

*/

int main(int argc, char *argv[]) {
	char *port;
	struct addrinfo addr_hints, *addr_result;
	char buffer[MAX_CONNECTIONS][BUFF_SIZE];
	int ret, msgsock, i, j, free_id, rval;

	//Inicjalizacja stosu z wolnymi id
	for (i = MAX_CONNECTIONS; i > 0; i--) {
		available_id.push(i);
	}

	//Inicjalizacja tablicy połączeń
	for (i = 0; i < MAX_CONNECTIONS; ++i) {
		connections[i].fd = -1;
		connections[i].events = POLLIN;
		connections[i].revents = 0;
	}

	//Inicjalizacja tablicy buforów
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		for (j = 0; j < BUFF_SIZE; j++) {
			buffer[i][j] = 0;
			player_replies[i][j] = 0;
		}
	}

	//Weryfikacja parametrów programu
	//TODO: Na razie weryfikuje tylko ilość
	if (argc == 2) {
		port = argv[1];
		regex valid_port{"^[1-9]\\d*$"};
		if (!regex_match(port, valid_port)) {
			cerr << "Invalid port number" << endl;
			return 1;
		}
	} else if (argc == 1) {
		port = "0";
	} else {
		cerr << "Wrong parameters" << endl;
		return 1;
	}

	/*Inicjalizacja zmiennych*/
	memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_flags = AI_PASSIVE;
	addr_hints.ai_family = AF_UNSPEC;
	addr_hints.ai_socktype = SOCK_STREAM;
	addr_hints.ai_protocol = IPPROTO_TCP;

	getaddrinfo(NULL, port, &addr_hints, &addr_result);

	connections[0].fd = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);

	if (connections[0].fd < 0) {
		cerr << "Socket error" << endl;
		return 0;
	}

	if (bind(connections[0].fd, addr_result->ai_addr, addr_result->ai_addrlen) < 0) {
		cerr << "Bind errror" << endl;
		return 0;
	}

	//Wypisz port jeśli nie podano go w argumentach programu
	if (argc == 1) {
		if (getsockname (connections[0].fd, (struct sockaddr*)addr_result->ai_addr,
                   	(socklen_t*)&addr_result->ai_addrlen) < 0) {
    			cerr << "Getting socket port error" << endl;
			return 0;
  		}
  		printf("%d\n",ntohs(get_in_port((struct sockaddr *)addr_result->ai_addr)));
	}

  	if (listen(connections[0].fd, 25) < 0) {
    		cerr << "Listen error" << endl;
		return 0;
  	}

	do {
		for (i = 0; i < MAX_CONNECTIONS; ++i)
      			connections[i].revents = 0;

		ret = poll(connections, MAX_CONNECTIONS, -1);

		if (ret < 0) {
			cerr << "Poll error" << endl;
			return 0;
		} else {
			//W przypadku nowego połączenia telnetowego otwórz nowe łącze tcp
			if (connections[0].revents & POLLIN) {
				msgsock = accept(connections[0].fd, (struct sockaddr*)0, (socklen_t*)0);
				if (msgsock < 0) {
					cerr << "Accept error" << endl;
					return 0;
				} else {
					//Przydziel indeks w tablicy połączeń nowemu połączeniu
					if (!available_id.empty()) {
						free_id = available_id.top();
						available_id.pop();
						connections[free_id].fd = msgsock;
					} else {
						cerr << "No more available ids; Too many connections" << endl;
					}
				}
			}

			//Szukamy indeksu innego niż 0 na którym zaszło jakieś zdarzenie
			for (i = 1; i < MAX_CONNECTIONS; i++) {
				if (connections[i].fd != -1 && (connections[i].revents & (POLLIN | POLLERR))) {
					rval = read(connections[i].fd, buffer[i], BUFF_SIZE);
					if (rval < 0) {
						cerr << "Read error" << endl;
						return 0;
					}

					//Sprawdzamy czy znaleziony indeks to id działającego playera czy numer połączenia telnetowego
					if (player_id.find(i) != player_id.end()) {
						
					} else {
						process_telnet_input(buffer[i], i);
						memset(buffer[i], 0, BUFF_SIZE);
					}
				}
			}

		}

	} while(true);
}

