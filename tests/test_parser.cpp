#include <gcode_parser.h>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>

int main() {
    const char* path = "test.gcode";
    std::ofstream out(path);
    out << "; full line comment\n";
    out << "   ; leading whitespace comment\n";
    out << "G0 X0 Y0\n";
    out << "   G1 X1 Y1\n";
    out << "\n";
    out.close();

    std::vector<stratum::GCodeCommand> cmds;
    stratum::parse_file(path, std::back_inserter(cmds));

    assert(cmds.size() == 2);
    assert(cmds[0].command == "G0");
    assert(cmds[0].arguments.size() == 2);
    assert(cmds[0].arguments[0] == "X0");
    assert(cmds[0].arguments[1] == "Y0");
    assert(cmds[1].command == "G1");
    assert(cmds[1].arguments.size() == 2);
    assert(cmds[1].arguments[0] == "X1");
    assert(cmds[1].arguments[1] == "Y1");

    std::remove(path);
    return 0;
}
