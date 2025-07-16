;--- Photocuring Pass: LED Above (Shining Downward) ---
; All distances in millimeters; absolute coordinates
G21             ; set units = mm
G90             ; absolute positioning

; --- Setup: move to safe height above resin surface ---
G0 X0 Y0 Z10 F6000   ; rapid move to home over resin, 10 mm above

; --- Turn LED ON at full brightness ---
; (here pin 6 drives the LED; adjust for your board)
M42 P6 S255      ; set digital pin 6 to 255/255 (fully on)
G4 P100          ; dwell 100 ms to let LED stabilize

; --- Traverse a simple square pattern at Z=5 mm above resin ---
G0 Z5 F3000      ; move down to curing height
G1 X50 Y0 F1500  ; move to (50,0)
G1 X50 Y50       ; move to (50,50)
G1 X0 Y50        ; move to (0,50)
G1 X0 Y0         ; return to origin

; --- Turn LED OFF ---
M42 P6 S0        ; LED off
G4 P50           ; brief pause

; --- Finish ---
G0 Z10 F6000     ; retract to safe height
M30              ; end of program
