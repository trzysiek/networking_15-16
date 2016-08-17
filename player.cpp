#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>
#include <string>

const unsigned int PARAMETERS_NR = 6;

int main(int argc, char* argv[]) {
    using std::cout;
    using std::endl;

    try {
        std::string host;
        std::string path;
        int servPortNum;
        int ourPortNum;
        std::string outputFile;
        std::string md;

        po::options_description desc("OPTIONS");
        desc.add_options()
            ("host", po::value<std::string>(&host), "host server name")
            ("path", po::value<std::string>(&path), "path to resource, commonly just /")
            ("r-port", po::value<int>(&servPortNum), "server's port")
            ("file", po::value<std::string>(&outputFile), "path to file, to which audio is saved"
                  ", or '-' when audio on stdout")
            ("m-port", po::value<int>(&ourPortNum), "UDP port on which we listen")
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
            cout << servPortNum << endl;
            cout << outputFile << endl;
            cout << ourPortNum << endl;
            cout << md << endl;

            // dodac jakies sprawdzenia czy md, porty poprawne, itp.
            // run program
        }
    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return 1;
    }    
    return 0;
}
