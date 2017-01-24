#ifndef __EJECT_TIMEOUT_TRACKER_H__
#define __EJECT_TIMEOUT_TRACKER_H__

#include <Arduino.h>

class EjectTimeoutTracker {
public:
	EjectTimeoutTracker():
		_check(false)
	{
	}

	__attribute__((always_inline)) inline
	void begin(uint32_t const timeout_us) {
		_timeout_us = timeout_us;
	}

	__attribute__((always_inline)) inline
	void start(uint32_t const now = micros()) {
		_start_us = now;
		_check = true;
	}

	__attribute__((always_inline)) inline
	void stop() {
		_check = false;
	}

	__attribute__((always_inline)) inline
	bool trigger(uint32_t const now = micros()) {
		if (_check && now - _start_us > _timeout_us) {
			_check = false;
			return true;
		}
		return false;
	}

private:
	uint32_t _timeout_us;
	uint32_t _start_us;
	bool _check;
};

#endif
