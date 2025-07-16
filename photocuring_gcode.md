# Photocuring G-code Format

This document describes the structure of a simple photocuring pass for LED-based resin printers. The examples used in the tests are included as reference.

## Key Characteristics

* **Units and Positioning**
  * `G21` sets millimeter units.
  * `G90` selects absolute coordinates.
* **Movement Commands**
  * `G0` performs rapid positioning moves, typically for jogging between locations.
  * `G1` performs coordinated moves at a programmed feed rate.
* **LED Control**
  * `M42 P<pin> S<value>` sets the output level of a digital pin. A value of `0` turns the LED off while `255` represents full intensity. Some firmwares also support `M106`/`M107` for fan/LED control.
  * `G4 P<ms>` inserts a dwell in milliseconds, allowing the LED time to stabilize after switching.
* **Heights**
  * Positive Z values position the LED above the XY plane (shining downward).
  * Negative Z values place the LED below the plane (shining upward through a transparent build plate).
* **Program End**
  * `M30` marks the end of the program.

## Example Files

Two example programs demonstrate the approach:

1. **`photocuring_above.gcode`** – The LED is mounted above the XY plane. The program moves to a safe height, turns the LED on, traces a square at `Z=5`, and then turns the LED off.
2. **`photocuring_below.gcode`** – The LED is mounted below the plane and shines upward through a transparent build plate. The pattern is identical but uses negative Z heights.

Both files reside in `tests/gcode/` and are parsed in the automated tests.

These examples illustrate a minimal workflow; real printers may require additional initialization and cleanup commands.
