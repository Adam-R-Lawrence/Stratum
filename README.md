# Stratum

Stratum is a simple C++20 project for experimenting with LED photopolymerization
printing. It provides a header-only library for generating G-code from ASCII STL
files and for parsing existing G-code. The generated G-code assumes a moving LED
light source, making it suitable for LED-based resin printers.

## Building

This project uses CMake. A typical build might look like:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will produce the `example` executable which shows basic usage of the
library.

## Usage

The `examples/example.cpp` file demonstrates generating G-code from a
hypothetical `example.stl` file and then parsing an `example.gcode` file. The
headers now live in the `src/` directory and can be included as:

```cpp
#include <gcode_parser.h>
#include <gcode_generator.h>
```

## License

Stratum is licensed under the [MIT License](LICENSE).
