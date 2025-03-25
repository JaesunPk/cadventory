#include "executeCommand.h"
#include <iostream>
#include <cstdio>  
#include <stdexcept>

std::string executeCommand(const std::string& command) {
    std::string result;
    char buffer[128];

    FILE* pipe;
#ifdef _WIN32
    pipe = _popen(command.c_str(), "r");
#else
    pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        throw std::runtime_error("Failed to execute command.");
    }

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}
