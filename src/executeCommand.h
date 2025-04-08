#ifndef EXECUTE_COMMAND_H
#define EXECUTE_COMMAND_H

#include <string>

// Function declaration (only declare here, do NOT define)
std::string executeCommandNoWindow(const std::string& command);

std::string executeCommandNoWindowWithRedirection(const std::string& command,
    const std::string& inputFile,
    const std::string& outputFile);

#endif // EXECUTE_COMMAND_H