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
    Stratum::generate_from_stl(path, 1.0, curve, std::back_inserter(gcode));

    assert(gcode.size() >= 6);
    assert(gcode[0] == "; Begin G-code generated from STL");
    assert(gcode[1] == "G21");
    assert(gcode[2] == "G90");
    assert(gcode.back() == "; End G-code");

    std::filesystem::remove(path);
    return 0;
}
