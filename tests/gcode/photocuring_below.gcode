;--- Photocuring Pass: LED Below (Shining Upward) ---
; Uses a transparent build plate; LED on pin 7 shines up through it
G21             ; set units = mm
G90             ; absolute positioning

; --- Setup: go to start height below build plate ---
G0 X0 Y0 Z-5 F6000   ; rapid move to (0,0) at Z = -5 mm

; --- Turn LED ON at 80% brightness ---
M42 P7 S204      ; 204/255 ~ 80% duty cycle
G4 P100          ; let LED stabilize

; --- Draw same square pattern at Z = -2 mm below plane ---
G0 Z-2 F3000     ; move up to -2 mm
G1 X50 Y0 F1500  ; move to (50,0)
G1 X50 Y50       ; move to (50,50)
G1 X0 Y50        ; move to (0,50)
G1 X0 Y0         ; back to origin

; --- Turn LED OFF ---
M42 P7 S0        ; LED off
G4 P50           ; brief pause

; --- Finish ---
G0 Z-5 F6000     ; retract LED downward
M30              ; end of program
