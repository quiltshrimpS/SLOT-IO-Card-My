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

FRAM_MB85RC_I2C fram(MB85RC_ADDRESS_A000, true, /* WP */ A7, 16 /* kb */);
WreckedSPI< /* MISO */ 7, /* MOSI */ 2, /* SCLK_MISO */ 8, /* SCLK_MOSI */ 3, /* MODE_MISO */ 2, /* MODE_MOSI */ 0 > spi;

union {
    uint8_t bytes[3];
    struct OutPort port;
} out;

union {
    uint8_t bytes[3];
    struct InPort port;
} in;


bool do_print = false;
bool do_send = false;
int32_t inserted = 0;
int32_t ejected = 0;

uint8_t to_eject = 0;

// counter is 150 CPS, each cycle is 6.66ms, 50% duty = 3.33ms HIGH then 3.33ms LOW.
// we round it to 3.5ms to give it a bit buffer
Pulse<3500, 3500> pulse_counter_score;
Pulse<3500, 3500> pulse_counter_wash;
Pulse<3500, 3500> pulse_counter_insert;
Pulse<3500, 3500> pulse_counter_eject;

Debounce<LOW, 5000> debounce_banknote(
	nullptr,
	[] () {

	}
);

Debounce<HIGH, 5000> debounce_eject(
	nullptr,
	[] () {
		++ejected;
		pulse_counter_eject.pulse(1);
		// FIXME: had to issue stop early, or inertia ejects an extra coin
		if (to_eject-- < 2) {
			out.port.ssr1 = false; // pull LOW to stop the motor
			do_send = true;
		}
		do_print = true;
	}
);

Debounce<LOW, 5000> debounce_insert_1(
	nullptr,
	[] () {
		++inserted;
		pulse_counter_insert.pulse(1);
		do_print = true;
	}
);

Debounce<LOW, 5000> debounce_insert_2(
	nullptr,
	[] () {
		++inserted;
		pulse_counter_insert.pulse(1);
		do_print = true;
	}
);

Debounce<LOW, 5000> debounce_insert_3(
	nullptr,
	[] () {
		++inserted;
		pulse_counter_insert.pulse(1);
		do_print = true;
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
    Serial.begin(230400);

    Wire.begin(); // for FRAM
    fram.begin();

	// populate the debouncers
	debounce_banknote.begin(in.port.sw20, now);
	debounce_eject.begin(in.port.sw11, now);
	debounce_insert_1.begin(in.port.sw12, now);
	debounce_insert_2.begin(in.port.sw13, now);
	debounce_insert_3.begin(in.port.sw14, now);
}

void loop() {
    uint32_t t1, t2, t3;

    t1 = micros();
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.receive();
    in.bytes[1] = spi.receive();
    in.bytes[2] = spi.receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);
    t2 = micros();
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
	t3 = micros();

    if (do_send) {
		do_send = false;
		uint32_t t5, t6;
        t5 = micros();
        fastDigitalWrite(PIN_LATCH_OUT, LOW);
        spi.send(out.bytes[0]);
        spi.send(out.bytes[1]);
        spi.send(out.bytes[2]);
        fastDigitalWrite(PIN_LATCH_OUT, HIGH);
        t6 = micros();

        Serial.print(millis());
        Serial.print(": out = ");
        Serial.print((int) (out.bytes[0]), BIN);
        Serial.print(" ");
        Serial.print((int) (out.bytes[1]), BIN);
        Serial.print(" ");
        Serial.print((int) (out.bytes[2]), BIN);
        Serial.print(", inserted = ");
        Serial.print(inserted);
        Serial.print(", ejected = ");
        Serial.print(ejected);
		Serial.print(", debounce took ");
		Serial.print(t3 - t2);
		Serial.print("us, receive took ");
		Serial.print(t2 - t1);
        Serial.print("us, send took ");
		Serial.print(t6 - t5);
        Serial.println("us");
    }

    if (do_print) {
		do_print = false;
        Serial.print(millis());
        Serial.print(": in = ");
        Serial.print((int) (in.bytes[0]), BIN);
        Serial.print(" ");
        Serial.print((int) (in.bytes[1]), BIN);
        Serial.print(" ");
        Serial.print((int) (in.bytes[2]), BIN);
        Serial.print(", inserted = ");
        Serial.print(inserted);
        Serial.print(", ejected = ");
        Serial.print(ejected);
		Serial.print(", debounce took ");
		Serial.print(t3 - t2);
		Serial.print("us, receive took ");
		Serial.print(t2 - t1);
        Serial.println("us");
    }
}
