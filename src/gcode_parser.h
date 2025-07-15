#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace stratum {

struct GCodeCommand {
    std::string command;
    std::vector<std::string> arguments;
};

inline GCodeCommand parse_line(const std::string& line) {
    std::istringstream iss(line);
    GCodeCommand cmd;
    iss >> cmd.command;
    std::string arg;
    while (iss >> arg) {
        cmd.arguments.push_back(arg);
    }
    return cmd;
}

template <typename OutputIt>
void parse_file(const std::string& path, OutputIt out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            *out++ = parse_line(line);
        }
    }
}

} // namespace stratum
