#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace Stratum {

// Generates G-code comments from the lines of an ASCII STL file.
// Throws std::runtime_error if the file cannot be opened.
template <typename OutputIt>
void generate_from_stl(const std::filesystem::path& stl_path, OutputIt out) {
    std::ifstream file(stl_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + stl_path.string());
    }
    std::string line;
    *out++ = "; Begin G-code generated from STL";
    while (std::getline(file, line)) {
        if (!line.empty()) {
            *out++ = std::string("; ") + line;
        }
    }
    *out++ = "; End G-code";
}

} // namespace stratum
