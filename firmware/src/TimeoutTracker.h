#ifndef __EJECT_TIMEOUT_TRACKER_H__
#define __EJECT_TIMEOUT_TRACKER_H__

#include <Arduino.h>

class TimeoutTracker {
public:
	TimeoutTracker(
		#if defined(DEBUG_SERIAL)
		const char * const name
		#endif
	):
		#if defined(DEBUG_SERIAL)
		_name(name),
		#endif
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
		#if defined(DEBUG_SERIAL)
		if (_check) {
			DEBUG_SERIAL.print(F("90,"));
			DEBUG_SERIAL.print(_name);
			DEBUG_SERIAL.print(F(" stopped within "));
			DEBUG_SERIAL.print(micros() - _start_us);
			DEBUG_SERIAL.print(F("us;"));
		}
		#endif
		_check = false;
	}

	__attribute__((always_inline)) inline
	bool trigger(uint32_t const now = micros()) {
		if (_check && now - _start_us > _timeout_us) {
			#if defined(DEBUG_SERIAL)
			DEBUG_SERIAL.print(F("90,"));
			DEBUG_SERIAL.print(_name);
			DEBUG_SERIAL.print(F(" timedout after "));
			DEBUG_SERIAL.print(now - _start_us);
			DEBUG_SERIAL.print(F("us;"));
			#endif
			_check = false;
			return true;
		}
		return false;
	}

private:
	#if defined(DEBUG_SERIAL)
	const char * const _name;
	#endif
	uint32_t _timeout_us;
	uint32_t _start_us;
	bool _check;
};

#endif
