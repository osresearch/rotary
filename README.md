![Outbound call state machine](https://farm4.staticflickr.com/3694/32869954060_f3c01daa31_z_d.jpg)

This is an extension on Adafruit's [`FONAtest.ino`](https://github.com/adafruit/Adafruit_FONA/blob/master/examples/FONAtest/FONAtest.ino)
example code to allow it to be used with a rotary phone.  The Teensy
2.0 decodes the pulses and implements the state machine to allow you to
make place phone calls.  In-bound calls are not yet working; the ringer
circuit is a work in progress.

Wiring
===
![Inside the phone](https://farm3.staticflickr.com/2808/32833764510_fcd14da721_z_d.jpg)

* Teensy Gnd -> FONA Ground, FONA Key, Phone F (blue?)
* Teensy Vcc -> FONA Vio
* Teensy Pin 0 -> FONA Reset
* Teensy Pin 1 -> Phone RR (green on the dial), rotary dial
* Teensy Pin 2 -> Phone L1 (RJ45 green), hook relay
* Teensy Pin 7 -> FONA RX
* Teensy Pin 8 -> FONA TX

You will need a computer plugged into the Teensy right now
since it doesn't initialize the FONA until the USB serial is
connected.  This will need to be fixed for standalone operation.

Todo
===
![Dialup internet](https://farm4.staticflickr.com/3845/32372052844_b7e6990ddc_z_d.jpg)

* Incoming call state machine.
* Ringer driver circuit. Looks like 15V will engage the bell solenoid, so a simple charge pump and low frequency PWM should do it.
* PCB design to mount FONA, Teensy and wire headers for the phone connections
* Package everything to fit into the phone.
* Buy a red phone, cut a hole for USB charging cable.
* Teensy powered from the battery?  Battery switch?
* Figure out why 300 baud isn't working with the FONA as well as it did with my cellphone.
