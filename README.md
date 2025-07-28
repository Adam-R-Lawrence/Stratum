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

`generateGCode` takes an ASCII STL file along with either a
`Stratum::LCDConfig` or `Stratum::SLAConfig` structure describing the
printer setup. The resulting G-code is written through an output
iterator.  Masks for each layer are produced automatically and stored as
1‑bit PNG images.  `parseFile` reads an existing G-code file and
produces a sequence of `Stratum::GCodeCommand` objects.  Both functions
throw `std::runtime_error` if the requested file cannot be opened.

## License

Stratum is licensed under the [MIT License](LICENSE).
