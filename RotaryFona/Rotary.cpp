/** \file
 * Rotary phone state machine.
 *
 * Designed specifically to work with the Adafruit FONA
 *
 * The rotary phone uses one input pin to detect when the
 * handset has gone "off hook" and another to detect the
 * pulses of the rotary dial.  The ringer is not yet
 * supported.
 * 
 * BSD license, all text above must be included in any redistribution
 */

#include <Arduino.h>
#include "Rotary.h"

///// XXX: TODO need to make this use TimerOne now that we're using pin 11
#include "TimerThree.h"

// todo move this into the constructor
#define PULSE_DIAL	1 // rotary pin, normally closed
#define PULSE_HOOK	2 // off hook == 0, on hook == 1
#define PULSE_RING	6 // from the fona, when there is an incoming call
#define ROTARY_PWM	11 // pwm output for charge pump
#define ROTARY_RINGER	A1 // mosfet to dump voltage to the ringer

typedef enum {
	ROTARY_INIT	= 0,
	ROTARY_ONHOOK,
	ROTARY_ONHOOK_CHECK_STATUS,
	ROTARY_RINGING,
	ROTARY_RINGING_WAIT,
	ROTARY_ANSWERED,
	ROTARY_PLAY_DIALTONE,
	ROTARY_WAIT_FIRST_PULSE,
	ROTARY_PLAY_ERRTONE,
	ROTARY_WAIT_ONHOOK,
	ROTARY_PULSE_COUNT = 100,
	ROTARY_PULSE_FALLING,
	ROTARY_PULSE_IGNORE,
	ROTARY_PULSE_WAIT,
	ROTARY_ADD_DIGIT,
	ROTARY_DIAL_NUMBER,
	ROTARY_ON_CALL = 200,
	ROTARY_CALL_PULSE_COUNT,
	ROTARY_CALL_PULSE_FALLING,
	ROTARY_CALL_PULSE_IGNORE,
	ROTARY_CALL_PULSE_WAIT,
	ROTARY_CALL_DIAL_DIGIT,
	ROTARY_CALL_CHECK_STATUS,
	ROTARY_CALL_OVER,
} rotary_state_t;


void Rotary::begin()
{
	// Pull up on the rotary pulse pins
	pinMode(PULSE_DIAL, INPUT_PULLUP);
	pinMode(PULSE_HOOK, INPUT_PULLUP);
	pinMode(PULSE_RING, INPUT_PULLUP);

	// charge pump ringer parameters
	//Timer3.initialize(12); // microsecond pulse width
	pwm = 0;
	ringer_voltage = 400;

	// disable the ringer mosfet
	pinMode(ROTARY_RINGER, OUTPUT);
	digitalWrite(ROTARY_RINGER, 0);

	// disable the pwm charge pump mosfet
	pinMode(ROTARY_PWM, OUTPUT);
	digitalWrite(ROTARY_PWM, 0);

	new_state(ROTARY_INIT);
}

int Rotary::new_state(int new_state_val)
{
	if (state == new_state_val)
		return 0;

	state = new_state_val;
	last_transition = millis();

	Serial.print(last_transition);
	Serial.print(" state=");
	Serial.println(state);
	return state;
}


void Rotary::play_tone(int tone, int len)
{
	fona.playToolkitTone(tone, len);
}

void Rotary::stop_tone()
{
	fona.sendCheckReply(F("AT+STTONE=0"), F("OK"));
}

int Rotary::onhook()
{
	return digitalRead(PULSE_HOOK) != 0;
}

int Rotary::dial()
{
	return digitalRead(PULSE_DIAL) != 0;
}

int Rotary::ring()
{
	return digitalRead(PULSE_RING) == 0;
}

void Rotary::loop()
{
	state_machine();
}


void Rotary::bell(int state)
{
	digitalWrite(ROTARY_RINGER, state);
}


void Rotary::charge_pump(
	unsigned target
)
{
	const unsigned max_pwm = 990;
	const unsigned min_pwm = 100;

	if (target == 0)
	{
		// disable the charge pump, close the output
		//Timer3.pwm(ROTARY_PWM, 0);
		digitalWrite(ROTARY_RINGER, 0);
		pwm = 0;
		return;
	}

	if (pwm == 0)
	{
		// enable the PWM and pre-charge the pump for some time
		pwm = 512;
		pinMode(ROTARY_RINGER, OUTPUT);
		digitalWrite(ROTARY_RINGER, 0);
		//Timer3.pwm(ROTARY_PWM, pwm);
		return;
	}

	
	// read the current value and compare it to our target
	// if we are above the target, shorten the pwm
	// if we are below the target, increase it
	const unsigned val = analogRead(0);
	if (val > target)
	{
		pwm = pwm - (pwm - min_pwm)/2;
	} else {
		pwm = pwm + (max_pwm - pwm)/2;
	}

	//Timer3.pwm(ROTARY_PWM, pwm);
}


int Rotary::state_machine(void)
{
	// compute the time since last transition
	const uint32_t now = millis();
	const uint32_t delta = now - last_transition;

	// all states have a reset when we go back on hook
	// except for the ringing states
	if(onhook()
	&& state != ROTARY_ONHOOK
	&& state != ROTARY_ONHOOK_CHECK_STATUS
	&& state != ROTARY_RINGING
	&& state != ROTARY_RINGING_WAIT)
	{
		charge_pump(0);
		stop_tone();
		fona.hangUp();
		new_state(ROTARY_ONHOOK);
	}

	switch(state)
	{
	case ROTARY_INIT:
		if(!onhook())
			new_state(ROTARY_PLAY_ERRTONE);
		return 0;

	default:
	case ROTARY_ONHOOK:
		pulses = 0;
		digits = 0;
		bell_count = 0;

		if(!onhook())
			return new_state(ROTARY_PLAY_DIALTONE);

		if(ring())
		{
			bell(1);
			return new_state(ROTARY_RINGING);
		}

		return 0;

	case ROTARY_RINGING:
		// If they pick up the phone, go to answered state
		if (!onhook())
			return new_state(ROTARY_ANSWERED);

		// if the phone stops ringing, go back to the
		// onhook state
		if (!ring())
		{
			charge_pump(0);
			return new_state(ROTARY_ONHOOK);
		}

		charge_pump(ringer_voltage);

		if (delta < 40)
			return 0;

		// alternate on and off to pulse the bell
		last_transition = now;
		bell(bell_count & 1);
		if (bell_count++ < 50)
			return 0;

		bell_count = 0;
		return new_state(ROTARY_RINGING_WAIT);

	case ROTARY_RINGING_WAIT:
		// this is the second of silence between rings
		// If they pick up the phone, go to answered state
		if (!onhook())
			return new_state(ROTARY_ANSWERED);

		// if the phone stops ringing, go back to the
		// onhook state
		if (!ring())
		{
			charge_pump(0);
			return new_state(ROTARY_ONHOOK);
		}

		charge_pump(ringer_voltage);

		if (delta < 800)
			return 0;

		// resume the bell
		bell(1);
		return new_state(ROTARY_RINGING);

	case ROTARY_ANSWERED:
		// disable the PWM for the charge pump
		// and turn off the external ringer.
		charge_pump(0);
		stop_tone();
		fona.pickUp();
		return new_state(ROTARY_ON_CALL);

	case ROTARY_PLAY_DIALTONE:
		play_tone(20, 10000);
		return new_state(ROTARY_WAIT_FIRST_PULSE);

	case ROTARY_PLAY_ERRTONE:
		play_tone(2, 10000);
		return new_state(ROTARY_WAIT_ONHOOK);

	case ROTARY_WAIT_ONHOOK:
		// literally do nothing until we go on hook
		return 0;

	case ROTARY_WAIT_FIRST_PULSE:
		// if they have been off hook for too long, error out
		if (delta > 10000)
			return new_state(ROTARY_PLAY_ERRTONE);

		if (!dial())
			return 0;

		// they have started dialing, switch states (to keep the
		// timer correct), then stop the dial tone
		new_state(ROTARY_PULSE_COUNT);
		stop_tone();
		return 0;

	case ROTARY_PULSE_COUNT:
		pulses++;

		// if we get too many pulses, abort this call
		if (pulses > 10)
			return new_state(ROTARY_PLAY_ERRTONE);

		return new_state(ROTARY_PULSE_IGNORE);

	case ROTARY_PULSE_IGNORE:
		if (delta < 15)
			return 0;
		return new_state(ROTARY_PULSE_FALLING);

	case ROTARY_PULSE_FALLING:
		// wait for the falling edge of the transition
		if (!dial())
			return new_state(ROTARY_PULSE_WAIT);
		return 0;

	case ROTARY_PULSE_WAIT:
		// if they spin the dial, start counting again
		if (dial())
			return new_state(ROTARY_PULSE_COUNT);

		// short timeout if we have received pulses,
		// in which case this is a new digit.
		if (delta > 200 && pulses != 0)
			return new_state(ROTARY_ADD_DIGIT);

		// long timeout if we have no pulses, in which case
		// we have the entire number and call dial it
		if (delta > 3000)
			return new_state(ROTARY_DIAL_NUMBER);
		return 0;

	case ROTARY_ADD_DIGIT:
		if (digits >= sizeof(rotary_number) - 1)
			return new_state(ROTARY_PLAY_ERRTONE);

		if (pulses == 10)
			pulses = 0;

		Serial.print("dialed ");
		Serial.println(pulses);

		rotary_number[digits++] = '0' + pulses;
		pulses = 0;
		return new_state(ROTARY_PULSE_WAIT);

	case ROTARY_DIAL_NUMBER:
		rotary_number[digits++] = '\0';
		Serial.println(rotary_number);
		fona.callPhone(rotary_number);

		pulses = 0;
		digits = 0;

		return new_state(ROTARY_ON_CALL);

	case ROTARY_ON_CALL:
		if (dial())
			return new_state(ROTARY_CALL_PULSE_COUNT);
		if (delta > 1000)
			return new_state(ROTARY_CALL_CHECK_STATUS);
		return 0;

	case ROTARY_CALL_CHECK_STATUS:
		// if they hang up, signal that we're done
		if (fona.getCallStatus() == 0)
			return new_state(ROTARY_CALL_OVER);

		return new_state(ROTARY_ON_CALL);

	case ROTARY_CALL_OVER:
		fona.playToolkitTone(5, 30000);
		return new_state(ROTARY_WAIT_ONHOOK);

	case ROTARY_CALL_PULSE_COUNT:
		pulses++;
		return new_state(ROTARY_CALL_PULSE_IGNORE);

	case ROTARY_CALL_PULSE_IGNORE:
		if (delta < 25)
			return 0;
		return new_state(ROTARY_CALL_PULSE_FALLING);

	case ROTARY_CALL_PULSE_FALLING:
		if (!dial())
			return new_state(ROTARY_CALL_PULSE_WAIT);
		return 0;

	case ROTARY_CALL_PULSE_WAIT:
		if (delta > 200)
			return new_state(ROTARY_CALL_DIAL_DIGIT);
		if (dial())
			return new_state(ROTARY_CALL_PULSE_COUNT);
		return 0;

	case ROTARY_CALL_DIAL_DIGIT:
		if (pulses == 10)
			pulses = 0;

		Serial.print("digit ");
		Serial.println(pulses);
		fona.playDTMF(pulses + '0');
		pulses = 0;

		return new_state(ROTARY_ON_CALL);
	}
}


void Rotary::tune(void)
{
	// disable the ringer mosfet
	pinMode(ROTARY_RINGER, OUTPUT);
	digitalWrite(ROTARY_RINGER, 0);

	// disable the pwm charge pump mosfet
	pinMode(ROTARY_PWM, OUTPUT);
	digitalWrite(ROTARY_PWM, 0);

	// fake ring pin
	pinMode(A3, INPUT_PULLUP);

	Serial.begin(115200);
	int pulse_width = 12;
	//Timer3.initialize(pulse_width); // microsecond pulse width

	while(1)
	{
		int c = Serial.read();
		if (c == -1)
		{
			if (digitalRead(A3))
				continue;
			// they pulled the pin down
			c = '8';
		}

		if (c == '-' && pulse_width > 1)
		{
			pulse_width--;
			//Timer3.initialize(pulse_width); // microsecond pulse width
		} else
		if (c == '+' && pulse_width < 100)
		{
			pulse_width++;
			//Timer3.initialize(pulse_width); // microsecond pulse width
		} else
		if ('0' <= c && c <= '9')
		{
			int pwm = 0 + (1024-0) * (c - '0') / 10;
			Serial.print(pulse_width);
			Serial.print(" ");
			Serial.println(pwm);

			//Timer3.pwm(ROTARY_PWM, pwm);

			// pre-charge the pump for 100 ms
			digitalWrite(ROTARY_RINGER, 0);
			delay(100);

			// one second of ringing
			for(int i = 0 ; i < 20 ; i++)
			{
				// we want a 20 Hz ring,
				// so pull high for half that time,
				// then let it relax and recharge
				// for the other half (plus a bit)
				// this improves charge pump peformance
				// at low voltages
				digitalWrite(ROTARY_RINGER, 1);
				delay(22-10);
				digitalWrite(ROTARY_RINGER, 0);
				delay(22+10);
			}

			// disable the mosfets and charge pump
			//Timer3.pwm(ROTARY_PWM, 0);
			digitalWrite(ROTARY_RINGER, 0);
			digitalWrite(ROTARY_PWM, 0);

		} else
		{
			Serial.println("?");
		}
	}
}
