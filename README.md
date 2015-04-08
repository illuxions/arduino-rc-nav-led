arduino-rc-nav-led
==================

A brutally masacred hack of https://github.com/hasbridge/arduino-rc-nav-led to support NEO pixel LEDs.  All credit goes to Hashbridge for the code, I just tweaked it to support Neopixels.


RC aircraft navigation, anti-collision, and strobe lights using a Digispark Attiny85 or any other arduino. See a demo of the code in action at https://youtu.be/BsotRM70wjs or the original code in action https://youtu.be/sCwhbVWebv0

Current Features
----------------
* Landing light controlled via servo channel (can use a Y-harness off flaps or gear channel)
* Realistic dual anti-collision beacon, fades in/out to simulate rotating
* Realistic Double-blink strobe

Default pinout for the Digispark Attiny85
-------------------------------
The code is pre-configured for use with the Digispark ATTiny85 board. It should also run on any arduino board, but the pin numbers will need to be modified.

* P0 - not used
* P1 - not used
* P2 - DI pin for NeoPixel String
* P3 - Interrupt/ Landing lights pin
* P4 - Rnot used
* P5 - not used

* 5V - not used
* Gnd - Gnd from External 5v Regulator
* Vin - 5v from External Regulator

Each pixel can draw 60ma+  The regulator on the digispark is 150ma or 500ma depending on version, I highly recommend using an external regulator
