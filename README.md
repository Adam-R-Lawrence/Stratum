# Stratum

Stratum is a simple C++20 project for experimenting with LED photopolymerization printing. It provides a header-only library for generating G-code from ASCII STL files and for parsing existing G-code. The generated G-code assumes a moving LED light source, making it suitable for LED-based resin printers.

## Building

This project uses CMake. A typical build might look like:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

After building, run the automated tests with:

```bash
ctest
```

## Usage

The tests in the `tests/` directory demonstrate generating G-code from a hypothetical STL file and parsing a G-code file. The headers live in the `src/` directory and can be included as:

```cpp
#include <gcode_parser.h>
#include <gcode_generator.h>
```

Both APIs accept `std::filesystem::path` objects for file locations.

`generate_from_stl` accepts an LED spot radius and a set of `(depth,
exposure)` pairs describing the curing behavior. Additional parameters
specify the printing mode (`"LCD"` or `"SLA"`), the power level for the
light source, and the optional LED bitmask path used when operating in LCD
mode. The function determines the XY extents of the STL and emits a simple
raster scan for a single layer. The feed rate is interpolated from the
exposure curve. Both `generate_from_stl` and `parse_file` throw a
`std::runtime_error` if the specified file cannot be opened.

## License

Stratum is licensed under the [MIT License](LICENSE).
