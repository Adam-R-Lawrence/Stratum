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
    assert(cmds[2].command == "G0");
    assert(cmds[2].arguments.size() == 4);
    assert(cmds[2].arguments[0].letter == 'X');
    assert(cmds[2].arguments[0].value == 0.0);
    assert(cmds[2].arguments[1].letter == 'Y');
    assert(cmds[2].arguments[1].value == 0.0);
    assert(cmds[2].arguments[2].letter == 'Z');
    assert(cmds[2].arguments[2].value == 10.0);
    assert(cmds[2].arguments[3].letter == 'F');
    assert(cmds[2].arguments[3].value == 6000.0);

    cmds.clear();
    stratum::parse_file(below, std::back_inserter(cmds));
    assert(cmds.size() == 14);
    assert(cmds.front().command == "G21");
    assert(cmds.back().command == "M30");
    assert(cmds[2].command == "G0");
    assert(cmds[2].arguments.size() == 4);
    assert(cmds[2].arguments[0].letter == 'X');
    assert(cmds[2].arguments[0].value == 0.0);
    assert(cmds[2].arguments[1].letter == 'Y');
    assert(cmds[2].arguments[1].value == 0.0);
    assert(cmds[2].arguments[2].letter == 'Z');
    assert(cmds[2].arguments[2].value == -5.0);
    assert(cmds[2].arguments[3].letter == 'F');
    assert(cmds[2].arguments[3].value == 6000.0);

    return 0;
}
