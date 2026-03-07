#include "Application.h"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        Application app("Scientific Engine - Hydrostatic Sim", 1280, 720);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}