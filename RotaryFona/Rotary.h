#pragma once

#include "Adafruit_FONA.h"

class Rotary
{
public:
	Rotary(Adafruit_FONA &fona) :
		fona(fona),
		state(0)
	{
	}

	void begin();
	void loop();

	// helper to just tune the charge pump
	void tune();

private:
	Adafruit_FONA & fona;

	int state;
	uint32_t last_transition;
	unsigned pulses;
	unsigned digits;
	char rotary_number[32];

	unsigned bell_count;

	// run the state machine
	int state_machine();

	// transition to a new state
	int new_state(int next_state);

	// call the FONA tone playing codes
	void play_tone(int tone, int len);
	void stop_tone();

	// Enable the charge pump
	void charge_pump(unsigned target);

	// Route the charge pump output to ring the bell once
	void bell(int enable);

	// Returns true if the phone is "on hook"
	int onhook();

	// Returns true when the dial is sending pulses
	int dial();

	// Returns true when the Fona tells us that
	// the phone is "ringing".
	int ring();

	// current pwm output
	unsigned pwm;

	// target voltage on the ringer feedback
	unsigned ringer_voltage;
};
