#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <string_view>
#include <cctype>
#include <stdexcept>

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
        throw std::runtime_error("Failed to open " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        // Trim leading whitespace manually
        std::string_view view(line);
        while (!view.empty() && std::isspace(static_cast<unsigned char>(view.front()))) {
            view.remove_prefix(1);
        }

        if (view.empty() || view.front() == ';') {
            continue;
        }

        *out++ = parse_line(std::string(view));
    }
}

} // namespace stratum
