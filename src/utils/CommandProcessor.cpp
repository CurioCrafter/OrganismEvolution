#include "CommandProcessor.h"
#include <iostream>
#include <sstream>
#include <algorithm>

CommandProcessor::CommandProcessor(const std::string& commandFilePath)
    : commandFilePath(commandFilePath), lastFileSize(0) {
}

CommandProcessor::~CommandProcessor() {
}

void CommandProcessor::update() {
    std::ifstream file(commandFilePath);
    if (!file.is_open()) {
        return;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    long fileSize = file.tellg();

    // Check if file has new content
    if (fileSize > lastFileSize) {
        file.seekg(lastFileSize);
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                processCommand(line);
            }
        }
        lastFileSize = fileSize;
    }

    file.close();
}

void CommandProcessor::registerCommand(const std::string& name, std::function<void(const std::string&)> callback) {
    commands[name] = callback;
}

void CommandProcessor::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    // Get the rest of the line as arguments
    std::string args;
    std::getline(iss, args);
    if (!args.empty() && args[0] == ' ') {
        args = args.substr(1);
    }

    std::cout << "[Command] Received: " << cmd << " " << args << std::endl;

    auto it = commands.find(cmd);
    if (it != commands.end()) {
        it->second(args);
    } else {
        std::cout << "[Command] Unknown command: " << cmd << std::endl;
    }
}
