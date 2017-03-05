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

private:
	Adafruit_FONA & fona;

	int state;
	uint32_t last_transition;
	unsigned pulses;
	unsigned digits;
	char rotary_number[32];

	// run the state machine
	int state_machine();

	// transition to a new state
	int new_state(int next_state);

	// call the FONA tone playing codes
	void play_tone(int tone, int len);
	void stop_tone();

	// make the phone ring (once)
	void ring(int enable);

	// Returns true if the phone is "on hook"
	int onhook();

	// Returns true when the dial is sending pulses
	int dial();
};
