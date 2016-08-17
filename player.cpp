#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

const unsigned int PARAMETERS_NR = 6;
const unsigned int BUFFER_SIZE = 2000;

std::string host;
std::string path;
int servPort;
int ourPort;
std::string outputFile;
std::string md;

using std::endl;

std::string createRequest(std::string path, std::string meta) {
    return "GET " + path + " HTTP/1.0\r\n" +
           "Icy-MetaData:" + std::to_string(meta) + "\r\n" +
           "\r\n";
}

int karaj() {
	struct addrinfo hints, *res;
	int sockfd;
    int s;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;      // either ip4 or 6
	hints.ai_socktype = SOCK_STREAM;  // tcp
    // TODO: sprawdzic czy warto/trzeba dodac AI_PASSIVE w hints.flags?

    // get address
	if ((s = getaddrinfo(host.c_str(), std::to_string(servPort).c_str(), &hints, &res)) != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(s) << endl;
        return 1;
	}

	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        std::cerr << "socket error" << endl;
        return 1;
    }

	// connect!
	s = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (s < 0) {
        freeaddrinfo(res);
        std::cerr << "connect error" << endl;
        return 1;
    }
    freeaddrinfo(res);
    
    std::string request = createRequest(path, (md == "yes") ? "1" : "0");
    std::cout << request << endl;

    // send request to server
    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "send error" << endl;
        return 1;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // receive the answer
    s = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);

    // powinno dzialac
    std::cout << buffer << endl << "GG\n";
}

int main(int argc, char* argv[]) {
    using std::cout;

    try {
		// skopiowac tu globablne zmienne

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
    

        if (vm.size() != PARAMETERS_NR) {
            cout << "Usage: player OPTIONS\n";
            cout << desc;
            return 1;
        }
        else {
            // TODO: usunac debug
            cout << host << endl;
            cout << path << endl;
            cout << servPort << endl;
            cout << outputFile << endl;
            cout << ourPort << endl;
            cout << md << endl;

            // dodac jakies sprawdzenia czy md, porty poprawne, itp.
            // run program
            karaj();
        }
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return 1;
    }    
    return 0;
}
