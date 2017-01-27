#ifndef __PULSE_H__
#define __PULSE_H__

#include <Arduino.h>

#include "util.h"

static uint8_t const STATE_PAUSED = 0;
static uint8_t const STATE_HIGH = 1;
static uint8_t const STATE_LOW = 2;

template <uint32_t HIGH_US, uint32_t LOW_US>
class Pulse
{
public:
	Pulse():
		_pulses(0)
	{
	}

	__attribute__((always_inline)) inline
	void pulse(uint32_t pulses) {
		_pulses += pulses;
	}

	__attribute__((always_inline)) inline
	bool update(uint32_t now = micros())
	{
		// super easy state machine :-D
		if (likely(_state == STATE_PAUSED)) {
			// if in PAUSED and need to pulse, transit to HIGH
			if (_pulses != 0) {
				_start_us = now;
				_state = STATE_HIGH;
				return true;
			}
		} else {
			if (_state == STATE_HIGH) {
				// if in HIGH and checkpoint passed, transit to LOW
				if (now - _start_us > HIGH_US) {
					_state = STATE_LOW;
					_start_us = now;
					return true;
				}
			} else /* if (_state == STATE_LOW) */ {
				// if in LOW and checkpoint passed, transit to PAUSED
				if (now - _start_us > LOW_US) {
					_state = STATE_PAUSED;
					--_pulses;
				}
			}
		}
		return false;
	}

	__attribute__((always_inline)) inline
	bool get() {
		return _state == STATE_HIGH;
	}

private:
	uint32_t _start_us;
	uint32_t _pulses;
	uint8_t _state;
};

#endif
