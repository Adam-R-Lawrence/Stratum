#pragma once

#include <string>
#include <fstream>
#include <sstream>

namespace stratum {

template <typename OutputIt>
void generate_from_stl(const std::string& stl_path, OutputIt out) {
    std::ifstream file(stl_path);
    if (!file.is_open()) {
        return;
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
