#include <gcode_parser.h>
#include <gcode_generator.h>
#include <iostream>
#include <vector>
#include <iterator>
#include <filesystem>

int main() {
    try {
        std::vector<std::string> gcode;
        stratum::generate_from_stl(std::filesystem::path("example.stl"),
                                   std::back_inserter(gcode));
        for (const auto& line : gcode) {
            std::cout << line << '\n';
        }

        std::vector<stratum::GCodeCommand> commands;
        stratum::parse_file(std::filesystem::path("example.gcode"),
                            std::back_inserter(commands));
        std::cout << "Parsed " << commands.size() << " commands\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
