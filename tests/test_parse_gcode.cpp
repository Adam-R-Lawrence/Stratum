#include <gcode_parser.h>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <cassert>
#include <filesystem>

int main() {
    const std::filesystem::path path = "test.gcode";
    std::ofstream out(path);
    out << "; full line comment\n";
    out << "   ; leading whitespace comment\n";
    out << "G0 X0 Y0\n";
    out << "   G1 X1 Y1\n";
    out << "G0 X0 Y0 ; comment\n";
    out << "\n";
    out.close();

    std::vector<Stratum::GCodeCommand> cmds;
    Stratum::parseFile(path, std::back_inserter(cmds));

    assert(cmds.size() == 3);
    assert(cmds[0].command == "G0");
    assert(cmds[0].arguments.size() == 2);
    assert(cmds[0].arguments[0].letter == 'X');
    assert(cmds[0].arguments[0].value == 0.0);
    assert(cmds[0].arguments[1].letter == 'Y');
    assert(cmds[0].arguments[1].value == 0.0);
    assert(cmds[1].command == "G1");
    assert(cmds[1].arguments.size() == 2);
    assert(cmds[1].arguments[0].letter == 'X');
    assert(cmds[1].arguments[0].value == 1.0);
    assert(cmds[1].arguments[1].letter == 'Y');
    assert(cmds[1].arguments[1].value == 1.0);
    assert(cmds[2].command == "G0");
    assert(cmds[2].arguments.size() == 2);
    assert(cmds[2].arguments[0].letter == 'X');
    assert(cmds[2].arguments[0].value == 0.0);
    assert(cmds[2].arguments[1].letter == 'Y');
    assert(cmds[2].arguments[1].value == 0.0);

    std::filesystem::remove(path);
    return 0;
}
