/** \file
 * Rotary phone interface for pulse dialing.
 *
 * There is a signal for when they start dialing, but
 * we can just do it with timing.
 *
 * RR = pulse dial
 * F  = ground (for pulse dial)
 */

#define PULSE_DIAL	6  // 
#define PULSE_HOOK	10 // not used

void setup()
{
	Serial.begin(9600);

	// Pull up on the rotary pulse pins
	pinMode(PULSE_DIAL, INPUT_PULLUP);
	pinMode(PULSE_HOOK, INPUT_PULLUP);
}

void loop()
{
	static int count;
	static int last_dial;
	static uint32_t last_now;
	const uint32_t now = millis();
	const uint32_t delta = now - last_now;

	// but if the transition is longer than 200ms, then it is a new number
	if (delta > 200 && count != 0)
	{
		//Serial.print("--- ");
		Serial.println(count);
		count = 0;
	}

	// wait for a transition on the dial
	const int dial = digitalRead(PULSE_HOOK);
	if (dial == last_dial)
		return;
	// ignore transitions shorter than 5ms 
	if (delta < 25)
		return;

	// transition!
	if(0)
	{
		Serial.print(now);
		Serial.print(' ');
		Serial.println(dial);
	}

	last_dial = dial;
	last_now = now;

	// count the rising edge transitions
	if (dial)
		count++;
}
