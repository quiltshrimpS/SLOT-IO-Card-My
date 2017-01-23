/**
 * D2 - 74HC595 - SDI
 * D3 - 74HC595 - SRCLK
 * D4 - 74HC595 - RCLK
 * D5 - 74HC595 - nCLR
 * D6 - 74HC595 - nG
 * D7 - 74HC165 - QH
 * D8 - 74HC165 - CLK
 * D9 - 74HC165 - SH/nLD
 *
 * A4 - FRAM - SDA
 * A5 - FRAM - SCL
 * A7 - FRAM - WP
 */

#include <Arduino.h>
#include <DigitalIO.h>
#include <Wire.h>
#include <FRAM_MB85RC_I2C.h>

#include <CmdMessenger.h>

#include "WreckedSPI.h"
#include "Ports.h"
#include "Debounce.h"
#include "Pulse.h"
#include "Configuration.h"

FRAM_MB85RC_I2C fram(MB85RC_ADDRESS_A000, true, /* WP */ A7, 16 /* kb */);
WreckedSPI< /* MISO */ 7, /* MOSI */ 2, /* SCLK_MISO */ 8, /* SCLK_MOSI */ 3, /* MODE_MISO */ 2, /* MODE_MOSI */ 0 > spi;
Configuration conf(fram);

union {
    uint8_t bytes[3];
    struct OutPort port;
} out;

union {
    uint8_t bytes[3];
    struct InPort port;
} in, previous_in;

bool do_send = false;

Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_score;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_wash;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_insert;
Pulse<COUNTER_PULSE_DUTY_HIGH, COUNTER_PULSE_DUTY_LOW> pulse_counter_eject;

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_banknote(
	nullptr,
	[] () {
		conf.setCoinCount(TRACK_INSERT_3, conf.getCoinCount(TRACK_INSERT_3) + 1);
	}
);

Debounce<HIGH, DEBOUNCE_TIMEOUT> debounce_eject(
	nullptr,
	[] () {
		conf.setCoinCount(TRACK_EJECT, conf.getCoinCount(TRACK_EJECT) + 1);
		pulse_counter_eject.pulse(1);
		uint8_t to_eject = conf.getCoinsToEject(TRACK_EJECT);
		conf.setCoinsToEject(TRACK_EJECT, to_eject - 1);
		if (to_eject < 2) {
			out.port.ssr1 = false; // pull LOW to stop the motor
			do_send = true;
		}
	}
);

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_insert_1(
	nullptr,
	[] () {
		conf.setCoinCount(TRACK_INSERT_1, conf.getCoinCount(TRACK_INSERT_1) + 1);
		pulse_counter_insert.pulse(1);
	}
);

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_insert_2(
	nullptr,
	[] () {
		conf.setCoinCount(TRACK_INSERT_2, conf.getCoinCount(TRACK_INSERT_2) + 1);
		pulse_counter_insert.pulse(1);
	}
);

Debounce<LOW, DEBOUNCE_TIMEOUT> debounce_insert_3(
	nullptr,
	[] () {
		conf.setCoinCount(TRACK_INSERT_3, conf.getCoinCount(TRACK_INSERT_3) + 1);
		pulse_counter_insert.pulse(1);
	}
);

static uint8_t const PIN_LATCH_OUT = 4; // for 74HC595
static uint8_t const PIN_LATCH_IN = 9;  // for 74HC165

void setup() {
	// FIXME: write the SPI as early as possible, because 74HC595 defaults to
	// FIXME: HIGH on power-on. pull it low as early as possible.
    fastPinConfig(5, OUTPUT, LOW); // 595 nCLR (active LOW), clear register
    fastPinConfig(6, OUTPUT, LOW); // 595 nG (active LOW), enable
    fastPinConfig(PIN_LATCH_OUT, OUTPUT, HIGH); // 595 latch
    fastPinConfig(PIN_LATCH_IN, OUTPUT, LOW);   // 165 latch
    spi.begin(); // for 74HC595 and 74HC165
    fastDigitalWrite(5, HIGH); // cleared 74HC595, put it back on.
	// read the initial states, and write the initial states.
    fastDigitalWrite(PIN_LATCH_OUT, LOW);
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.transfer(out.bytes[0]);
    in.bytes[1] = spi.transfer(out.bytes[1]);
    in.bytes[2] = spi.transfer(out.bytes[2]);
    fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    fastDigitalWrite(PIN_LATCH_IN, LOW);

	uint32_t now = micros(); // record the time early, for better accuracy

	// do the rest of the thing after we switch off the motor
    Serial.begin(UART_BAUDRATE);

    Wire.begin(); // for FRAM
	Wire.setClock(TWI_BAUDRATE);
    fram.begin();
	conf.begin();

	// populate the debouncers
	debounce_banknote.begin(in.port.sw20, now);
	debounce_eject.begin(in.port.sw11, now);
	debounce_insert_1.begin(in.port.sw12, now);
	debounce_insert_2.begin(in.port.sw13, now);
	debounce_insert_3.begin(in.port.sw14, now);

	previous_in.bytes[0] = in.bytes[0] | IN_MASK_0;
	previous_in.bytes[1] = in.bytes[1] | IN_MASK_1;
	previous_in.bytes[2] = in.bytes[2] | IN_MASK_2;
}

void loop() {
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.receive();
    in.bytes[1] = spi.receive();
    in.bytes[2] = spi.receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);
	uint32_t now = micros();
	debounce_banknote.feed(in.port.sw20, now);
	debounce_eject.feed(in.port.sw11, now);
	debounce_insert_1.feed(in.port.sw12, now);
	debounce_insert_2.feed(in.port.sw13, now);
	debounce_insert_3.feed(in.port.sw14, now);
	if (pulse_counter_score.update(now))
	{
		out.port.counter1 = pulse_counter_score.get();
		do_send = true;
	}
	if (pulse_counter_wash.update(now))
	{
		out.port.counter2 = pulse_counter_wash.get();
		do_send = true;
	}
	if (pulse_counter_insert.update(now))
	{
		out.port.counter3 = pulse_counter_insert.get();
		do_send = true;
	}
	if (pulse_counter_eject.update(now))
	{
		out.port.counter4 = pulse_counter_eject.get();
		do_send = true;
	}

	// rest of the keys are not debounced, we just send them to the PC if
	// anything changed.
	uint8_t masked[] = {
		in.bytes[0] & IN_MASK_0,
		in.bytes[1] & IN_MASK_1,
		in.bytes[2] & IN_MASK_2,
	};
	if (masked[0] != previous_in.bytes[0] ||
		masked[1] != previous_in.bytes[1] ||
		masked[2] != previous_in.bytes[2])
	{
		previous_in.bytes[0] = masked[0];
		previous_in.bytes[1] = masked[1];
		previous_in.bytes[2] = masked[2];
	}

    if (do_send) {
		do_send = false;
        fastDigitalWrite(PIN_LATCH_OUT, LOW);
        spi.send(out.bytes[0]);
        spi.send(out.bytes[1]);
        spi.send(out.bytes[2]);
        fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    }
}
