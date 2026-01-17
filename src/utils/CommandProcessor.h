#pragma once

#include <string>
#include <fstream>
#include <functional>
#include <map>

class CommandProcessor {
public:
    CommandProcessor(const std::string& commandFilePath);
    ~CommandProcessor();

    void update();
    void registerCommand(const std::string& name, std::function<void(const std::string&)> callback);

private:
    std::string commandFilePath;
    std::map<std::string, std::function<void(const std::string&)>> commands;
    long lastFileSize;

    void processCommand(const std::string& command);
};
