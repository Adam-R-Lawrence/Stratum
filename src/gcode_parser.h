#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <string_view>
#include <cctype>
#include <stdexcept>
#include <filesystem>

namespace Stratum {

struct Arg {
    char letter{};
    double value{};
};

struct GCodeCommand {
    std::string command;
    std::vector<Arg> arguments;
};

inline Arg parseArg(const std::string& token) {
    if (token.size() < 2) {
        throw std::runtime_error("Invalid G-code argument: " + token);
    }
    Arg arg;
    arg.letter = token[0];
    try {
        arg.value = std::stod(token.substr(1));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric value in argument: " + token);
    }
    return arg;
}

inline GCodeCommand parseLine(const std::string& line) {
    std::istringstream iss(line);
    GCodeCommand cmd;
    iss >> cmd.command;
    std::string token;
    while (iss >> token) {
        cmd.arguments.push_back(parseArg(token));
    }
    return cmd;
}

// Parses a G-code file and writes each command to the output iterator.
// Throws std::runtime_error if the file cannot be opened.
template <typename OutputIt>
void parseFile(const std::filesystem::path& path, OutputIt out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + path.string());
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

        auto comment_pos = view.find(';');
        if (comment_pos != std::string_view::npos) {
            view = view.substr(0, comment_pos);
        }

        while (!view.empty() && std::isspace(static_cast<unsigned char>(view.back()))) {
            view.remove_suffix(1);
        }

        if (view.empty()) {
            continue;
        }

        *out++ = parseLine(std::string(view));
    }
}

} // namespace Stratum
