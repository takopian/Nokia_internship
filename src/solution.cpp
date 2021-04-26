#include <iostream>
#include "table.hpp"


int main(int argc, char ** argv) {
    if (argc == 1) {
        std::cerr << "No arguments. Program takes one argument - .csv file.";
        return 0;
    }
    try {
        auto table = Table(argv[1]);
        table.print_table();
    } catch (std::exception& e) {
        std::cout << e.what();
    }
    return 0;
}
