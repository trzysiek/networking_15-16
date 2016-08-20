#include "player.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>


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
            // TODO: usunac debug
            printf("%s\n%s\n%d\n%s\n%d\n%s\n", host.c_str(), path.c_str(),
                    servPort, outputFile.c_str(), ourPort, md.c_str());

            // dodac jakies sprawdzenia czy md, porty poprawne, itp.
            connect_with_server(host, path, servPort, ourPort, md);
        }
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }    
    return 0;
}
