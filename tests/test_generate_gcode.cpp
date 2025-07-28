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
    out << "  facet normal 0 0 1\n";
    out << "    outer loop\n";
    out << "      vertex 0 0 0\n";
    out << "      vertex 1 0 1\n";
    out << "      vertex 0 1 0\n";
    out << "    endloop\n";
    out << "  endfacet\n";
    out << "endsolid test\n";
    out.close();

    Stratum::LCDConfig cfg;
    cfg.cols = 5;
    cfg.rows = 5;
    cfg.led_radius = 0.5;
    cfg.layer_height = 1.0;
    cfg.final_lift_mm = 0.0;
    cfg.png_dir = "layers_out";

    std::vector<std::string> gcode;
    Stratum::generateGCode(path, cfg, std::back_inserter(gcode));

    assert(gcode.size() >= 8);
    assert(gcode[0].rfind("; **** MSLA Print ****", 0) == 0);
    assert(gcode[1] == "G28");
    assert(gcode[2] == "G90");
    assert(gcode.back().rfind("; PNG layers stored in", 0) == 0);

    std::filesystem::remove(path);
    std::filesystem::remove_all(cfg.png_dir);
    return 0;
}
