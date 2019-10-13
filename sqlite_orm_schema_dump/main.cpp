#include <iostream>
#include <string>
#include <stdexcept>
#include <map>

void usage(const std::string& binary) {
    std::cout << "Usage:\n" << binary << " [-sql] SOURCE DEST\n"
              << "Options\n"
              << "-sql\t\tThe source contains sql instead of a sqlite db\n"
              << std::endl;
}

int main(int argc, char *argv[])
{
    try {
        bool useSql = false;
        if(argc < 3) {
            throw std::runtime_error("not enough arguments");
        } else if (argc > 3) {
            if(std::string(argv[1]) == "-sql") {
                useSql = true;
            } else {
                throw std::runtime_error("error whild parsing sql");
            }
        }
        std::string dest(argv[argc-1]);
        std::string src(argv[argc-2]);
        std::cout << "convert " << src << " to " << dest;

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        usage(argv[0]);
        return 1;
    }
    return 0;
}
