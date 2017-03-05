![Outbound call state machine](https://farm4.staticflickr.com/3694/32869954060_f3c01daa31_z_d.jpg)

This is an extension on Adafruit's `FONAtest.ino` example code to
allow it to be used with a rotary phone.  The Teensy 2.0 decodes
the pulses and implements the state machine to allow you to make
place phone calls.  In-bound calls are not yet working; the
ringer circuit is a work in progress.

Wiring:

* Teensy Gnd -> FONA Ground, FONA Key, Phone F (blue?)
* Teensy Vcc -> FONA Vio
* Teensy Pin 0 -> FONA Reset
* Teensy Pin 1 -> Phone RR (green on the dial), rotary dial
* Teensy Pin 2 -> Phone L1 (RJ45 green), hook relay
* Teensy Pin 7 -> FONA RX
* Teensy Pin 8 -> FONA TX

You will need a computer plugged into the Teensy right now
since it doesn't initialize the FONA until the USB serial is
connected.
