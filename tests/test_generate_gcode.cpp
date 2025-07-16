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

    std::vector<std::string> gcode;
    Stratum::generate_from_stl(path, std::back_inserter(gcode));

    assert(gcode.size() == 4);
    assert(gcode[0] == "; Begin G-code generated from STL");
    assert(gcode[1] == "; solid test");
    assert(gcode[2] == "; endsolid test");
    assert(gcode[3] == "; End G-code");

    std::filesystem::remove(path);
    return 0;
}
