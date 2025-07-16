#include <gcode_parser.h>
#include <vector>
#include <string>
#include <iterator>
#include <cassert>
#include <filesystem>

int main() {
    const std::filesystem::path above = "../tests/gcode/photocuring_above.gcode";
    const std::filesystem::path below = "../tests/gcode/photocuring_below.gcode";

    std::vector<stratum::GCodeCommand> cmds;
    stratum::parse_file(above, std::back_inserter(cmds));
    assert(cmds.size() == 14);
    assert(cmds.front().command == "G21");
    assert(cmds.back().command == "M30");

    cmds.clear();
    stratum::parse_file(below, std::back_inserter(cmds));
    assert(cmds.size() == 14);
    assert(cmds.front().command == "G21");
    assert(cmds.back().command == "M30");

    return 0;
}
