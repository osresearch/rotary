/** \file
 * Rotary phone interface for pulse dialing.
 *
 * There is a signal for when they start dialing, but
 * we can just do it with timing.
 */

#define PULSE_DIAL	6
#define PULSE_HOOK	10

void setup()
{
	Serial.begin(9600);

	// Pull up on the rotary pulse pins
	pinMode(PULSE_DIAL, INPUT_PULLUP);
	pinMode(PULSE_HOOK, INPUT_PULLUP);
}

void loop()
{
	static int last_dial;

	// wait for a transition on the dial
	const int dial = digitalRead(PULSE_HOOK);
	if (dial == last_dial)
		return;

	// transition!
	Serial.println(dial);
	last_dial = dial;
}
