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
#include <queue>
#include <vector>
#include <functional>
#include <iostream>
#include <boost/regex.hpp>
#include <time.h>

using namespace std;

const int MAX_CONNECTIONS = 1000;
const int BUFF_SIZE = 100;
const int NON_EXISTENT = 0;
const int AWAITING = 1;
const int RUNNING = 2;
const int START_MESSAGE = 0;
const int QUIT_MESSAGE = 1;
//Struktura na przechowywanie poleceń w kolejce
struct queue_data {
	int data_type;
	int id;
	//Ponizsze dane potrzebne tylko jesli queue_data jest typu START_MESSAGE
	string parameters;
	string hostname;
	string port;
	int index;
};

struct action_comparator {
	bool operator() (const pair<time_t, queue_data> &a, const pair<time_t, queue_data> &b) const {
		return a.first < b.first;
	}
};

struct pollfd connections[MAX_CONNECTIONS];
stack<int> available_id;
set<int> player_id;
map<int, vector<string>> player_replies;
set<pair<time_t, queue_data>, action_comparator> queued_actions; 
int players_state[MAX_CONNECTIONS];
bool awaiting_title[MAX_CONNECTIONS];
double poll_time = -1;

void actualize_poll_time() {
	auto it = queued_actions.begin();
	time_t now = time(0);
	poll_time = difftime(it->first, now)*1000;
}

int get_new_id() {
	int id = available_id.top();
	available_id.pop();
	player_id.insert(id);
	return id;
}

void release_id(int id) {
	auto it = player_id.find(id);
	player_id.erase(it);
	available_id.push(id);
}

void start_command(string command, string hostname, string port, int index, int id) {
	string message;
	//TODO: OGARNĄĆ STREAMA Z POPEN ŻEBY ZOBACZYĆ CZY PLAYER SIE NIE WYPIERDALA
	popen(command.c_str(), "r");
	//Zarezerwuj id dla nowego playera
	players_state[id] = RUNNING;
	//Stwórz UDP socketa dla nowego playera
	struct addrinfo addr_hints;
  	struct addrinfo *addr_result;
  	memset(&addr_hints, 0, sizeof addr_hints);
	addr_hints.ai_family = AF_UNSPEC;
	addr_hints.ai_socktype = SOCK_DGRAM;
	addr_hints.ai_flags = AI_PASSIVE;
	getaddrinfo(hostname.c_str(), port.c_str(), &addr_hints, &addr_result);
	int sockfd = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
	if (sockfd < 0) {
		cerr << "Socker error" << endl;
	}
	connect(sockfd, addr_result->ai_addr, addr_result->ai_protocol);
	connections[id].fd = sockfd;
	//Powiadom sesję telnetową o sukcesie TODO:BYĆ MOŻE TO POWINNO BYĆ WYSYŁANE PRZY UDANYM POLLOUCIE? ALBO PRZY JAKIMŚ TAM RETURNIE Z POPEN()
	message = "OK " + to_string(id);
	send(connections[index].fd, message.c_str(), message.length(), 0);
}

void message_for_player(int id, int telnet_index, string command) {
	if (player_id.find(id) == player_id.end()) {
		cerr << "Player of given id does not exist" << endl; //Może niech wypisuje to id, albo nawet wysyła to do sesji telnetowej?
	} else if ((command == "PAUSE" || command == "PLAY") && players_state[id] == RUNNING) {
		send(connections[id].fd, command.c_str(), command.length(), 0);
	} else if (command == "TITLE" && players_state[id] == RUNNING) {
		awaiting_title[id] = true;
		send(connections[id].fd, command.c_str(), command.length(), 0);
		//TODO:Zrealizować oczekiwanie na title? Maks 3 sekundy.
	} else if (command == "QUIT") {
		if (players_state[id] == RUNNING) {
			send(connections[id].fd, command.c_str(), command.length(), 0);
		} else if (players_state[id] == AWAITING) {
			players_state[id] = NON_EXISTENT;
		}
	}
}

void at_command() {
	
}

void process_telnet_input(char buff[], int index) {
	boost::regex start{"^START (?:([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) ((?:(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) (?:\\w*\\/)+ [1-9]\\d* (?:[\\w,\\s-]+\\.[A-Za-z3]{3}) ([1-9]\\d*) (?:no|yes))(?:\\r\\n|\\r|\\n)$"};
	boost::regex player_command{"^(PAUSE|PLAY|TITLE|QUIT) ([1-9]\\d*)(\\r\\n|\\r|\\n)$"};
	boost::regex at{"^AT ((?:[0-9]|0[0-9]|1[0-9]|2[0-3])\\.[0-5][0-9]) ([1-9]\\d*) (?:([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) ((?:(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))*|(?:\\d{1,3}(?:\\.\\d{1,3}){3})) (?:\\w*\\/)+ [1-9]\\d* (?:[\\w,\\s-]+\\.[A-Za-z3]{3}) ([1-9]\\d*) (?:no|yes))(?:\\r\\n|\\r|\\n)$"};
	boost::regex control_bytes{"\\xFF[\\xFB-\\xFF].|\\xFF[^\\xFB-\\xFF]"};
	boost::regex double_escape{"\\xFF\\xFF"};
	string single_escape("\xFF");
	boost::smatch match;
	string empty = "";
	string input(buff);
	int id;
	string message;

	//Usuń kontrolne bajty przysłane przez sesję telnetową TODO:Spróbować to przetestować jakoś
	boost::regex_replace(input, control_bytes, empty);
	boost::regex_replace(input, double_escape, single_escape);

	if (boost::regex_search(input, match, start)) {
		//Otwórz program player na odpowiednim hoście przez ssh
		string hostname = string(match[1].first, match[1].second); //DA SIE CHBYA PO PROSTU HOSTNAME(PARAMETRY)?
		string port = string(match[3].first, match[3].second);
		string cmd = "ssh " + hostname + " player " + string(match[2].first , match[2].second);
		//Wykonaj działania związane z poleceniem start
		id = get_new_id();
		start_command(cmd, hostname, port, index, id);
		//TODO:Wpisać player_replies i dać POLLOUT na sesje telnetowa o indeksie index????
		
	} else if (boost::regex_search(input, match, player_command)) {
		id = atoi(string(match[2].first, match[2].second).c_str());
		message = string(match[1].first, match[1].second);
		message_for_player(id, index, message);
		//TODO:1.Wpisać player_replies i dać POLLOUT JEŚLI TO NIE TITLE 2.Jeśli title to jak otrzymamy tytuł damy POLLOUT itd.

	} else if (boost::regex_search(input, match, at)) {
		//Wczytaj dane przesłane poleceniem AT
		string hostname = string(match[3].first, match[3].second);
		string port = string(match[5].first, match[5].second);
		string cmd = "ssh " + hostname + " player " + string(match[4].first, match[4].second); //Można dodać potem przekierowywanie stdout na stderr w ssh jeśli to samo będzie w START trza zrobic
		string player_life = string(match[2].first, match[2].second);
		string start_time = string(match[1].first, match[1].second);
		//Wczytaj czas przesłany poleceniem AT
		struct tm *ltm;
		time_t now = time(0);
		ltm = localtime(&now);
		strptime(start_time.c_str(), "%H.%M", ltm);
		time_t formatted_start_time = mktime(ltm);
		time_t end_time = formatted_start_time + 60*atoi(player_life.c_str());
		id = get_new_id();

		//Zakolejkuj polecenie startu playera
		queue_data start_data = {START_MESSAGE, id, cmd, hostname, port, index};
		pair<time_t, queue_data> data {formatted_start_time, start_data};
		queued_actions.insert(data);
		//Zakolejkuj polecenie wyłączenia playera
		queue_data quit_data = {QUIT_MESSAGE, id, "", "", "", 0};
		data = {end_time, quit_data};
		queued_actions.insert(data);
		//Zaktualizuj czas oczekiwania na wyjście z polla
		actualize_poll_time();
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
	3. WYPISYWAC ERRORA JAK SSH PRÓBUJE PYTAĆ O HASŁO
	4. ERRORY W FUNKCJACH INNYCH NIZ MAIN TEZ POWINNY WYWALAC PROGRAM
	5. ERRORY WYPISYWAĆ TELNETOWI
	7. IGNOROWANIE KOMUNIKATÓW DLA AT PLAYERÓW
	8. MOŻE SPRAWDZAĆ CZY PODANE PORTY SĄ WOLNE W SUMIE?? I W PLAYERZE I MASTERZE.

*/

int main(int argc, char *argv[]) {
	char *port;
	struct addrinfo addr_hints, *addr_result;
	char buffer[MAX_CONNECTIONS][BUFF_SIZE];
	int ret, msgsock, i, free_id, rval;

	//Inicjalizacja struktur
	for (i = MAX_CONNECTIONS; i > 0; i--) {
		available_id.push(i);
	}
	for (i = 0; i < MAX_CONNECTIONS; ++i) {
		connections[i].fd = -1;
		connections[i].events = POLLIN;
		connections[i].revents = 0;
	}
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		memset(buffer[i], 0, BUFF_SIZE);
	}
	memset(players_state, 0, MAX_CONNECTIONS);
	memset(awaiting_title, false, MAX_CONNECTIONS);

	//Weryfikacja parametrów programu
	if (argc == 2) {
		port = argv[1];
		boost::regex valid_port{"^[1-9]\\d*$"};
		if (!boost::regex_match(port, valid_port)) {
			cerr << "Invalid port number" << endl;
			return 1;
		}
	} else if (argc == 1) {
		port = (char *)"0";
	} else {
		cerr << "Wrong parameters" << endl;
		return 1;
	}

	//Przygotowanie zmiennych do nasłuchiwania połączeń
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

		ret = poll(connections, MAX_CONNECTIONS, poll_time);

		if (ret < 0) {
			cerr << "Poll error" << endl;
			return 0;
		} else if (ret > 0) {
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
				//Znaleziono dane do odczytu
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
				//Znaleziono połączeniu gotowe do zapisu
				if (connections[i].fd != -1 && (connections[i].revents & POLLOUT)) {

				}
			}

		} else if (ret == 0) {
			cout << "WLAZLEM TU GDZIE RET = 0 ZIOM" << endl;
			auto it = queued_actions.begin();
			if (it->second.data_type == START_MESSAGE) {
				start_command(it->second.parameters, it->second.hostname, it->second.port, it->second.index, it->second.id);
			} else if (it->second.data_type == QUIT_MESSAGE) {
				if (players_state[it->second.id] != NON_EXISTENT) {
					players_state[it->second.id] = NON_EXISTENT;
					release_id(it->second.id);
					string q = "QUIT";
					send(connections[it->second.id].fd, q.c_str(), q.length(), 0);
				}
			}
			queued_actions.erase(it);
		}

	} while(true);
}

