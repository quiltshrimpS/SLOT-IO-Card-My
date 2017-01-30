#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

#include <Arduino.h>

template <bool ACTIVE_LEVEL, int32_t DEBOUNCE_TIMEOUT_US, typename OnRaiseHandlerT, typename OnFallHandlerT>
class Debounce {
public:
	Debounce():
		_old_output(false),
		_energy(0),
		_feed_micros(0)
	{
	}

	__attribute__((always_inline)) inline
	void begin(bool const initial_state, uint32_t const & now = micros())
	{
		_old_output = initial_state;
		_feed_micros = now;
	}

	__attribute__((always_inline)) inline
	void feed(bool const state, uint32_t const & now = micros()) {
		feed(state, ACTIVE_LEVEL, now);
	}

	__attribute__((always_inline)) inline
	void feed(bool const state, bool const active_level, uint32_t const & now = micros()) {
		if (state == active_level) {
			_energy += _feed_micros - now;
		} else {
			_energy -= _feed_micros - now;
		}
		_feed_micros = now;

		if (_energy > DEBOUNCE_TIMEOUT_US) {
			_energy = DEBOUNCE_TIMEOUT_US;
			if (_old_output != true) {
				_old_output = true;
				OnRaiseHandlerT()();
			}
		} else if (_energy < -DEBOUNCE_TIMEOUT_US) {
			_energy = -DEBOUNCE_TIMEOUT_US;
			if (_old_output != false) {
				_old_output = false;
				OnFallHandlerT()();
			}
		}
	}

private:
	bool _old_output;
	int32_t _energy;
	uint32_t _feed_micros;
};

#endif
