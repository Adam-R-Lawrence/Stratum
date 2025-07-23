#include <gcode_generator.h>
#include <cassert>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

int main() {
    const std::filesystem::path path = "test.stl";
    std::ofstream out(path);
    out << "solid test\n";
    out << "endsolid test\n";
    out.close();

    std::vector<std::pair<double, double>> curve{{0.0, 1.0}, {1.0, 2.0}};

    std::vector<std::string> gcode;
    Stratum::generateFromStl(path,
                             1.0,
                             curve,
                             Stratum::PrintMode::LCD,
                             1.0,
                             std::back_inserter(gcode));

    assert(gcode.size() >= 13);
    assert(gcode[0] == "; Begin G-code generated from STL");
    assert(gcode[1] == "; Photopolymerization toolpath");
    assert(gcode[2] == "; Mode: LCD");
    assert(gcode[3] == "; Power: 1");
    assert(gcode[7] == "G21");
    assert(gcode[8] == "G90");
    assert(gcode[9].rfind("; Layer 0 bitmask:", 0) == 0);
    assert(gcode[10].rfind("G1", 0) == 0);
    assert(gcode[11].rfind("G1", 0) == 0);
    assert(gcode.back() == "; End G-code");

    std::filesystem::remove(path);
    return 0;
}
