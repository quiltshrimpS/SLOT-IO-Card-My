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

#define COIN_EJECT_LEVEL                (HIGH)

bool do_print = false;
bool do_send = false;
int32_t inserted = 0;
int32_t ejected = 0;

uint8_t to_eject = 0;

Debounce<COIN_EJECT_LEVEL, 5000> debounce_eject(
	nullptr,
	[] () {
		++ejected;
		// FIXME: had to issue stop early, or inertia ejects an extra coin
		if (to_eject-- < 2) {
			out.port.ssr5 = false;
			do_send = true;
		}
		do_print = true;
	}
);

Debounce<LOW, 5000> debounce_insert(
	nullptr,
	[] () {
		++inserted;
		do_print = true;
	}
);

Debounce<LOW, 5000> debounce_sw01(
	[] () {
		to_eject = 10;
		out.port.ssr5 = true;
		do_send = true;
	},
	nullptr
);

Debounce<LOW, 5000> debounce_sw02(
	nullptr,
	[] () {
		ejected = inserted = 0;
		do_print = true;
	}
);

static uint8_t const PIN_LATCH_OUT = 4; // for 74HC595
static uint8_t const PIN_LATCH_IN = 9;  // for 74HC165

void setup() {
    fastPinConfig(5, OUTPUT, LOW); // 595 nCLR (active LOW), clear register
    fastPinConfig(6, OUTPUT, LOW); // 595 nG (active LOW), enable
    fastPinConfig(PIN_LATCH_OUT, OUTPUT, HIGH); // 595 latch
    fastPinConfig(PIN_LATCH_IN, OUTPUT, LOW);   // 165 latch

    Serial.begin(115200);

    spi.begin(); // for 74HC595 and 74HC165

    Wire.begin(); // for FRAM
    fram.begin();

    // cleared 74HC595, put it back on.
    fastDigitalWrite(5, HIGH);

    // read the initial states, and write the initial states.
    fastDigitalWrite(PIN_LATCH_OUT, LOW);
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.transfer(out.bytes[0]);
    in.bytes[1] = spi.transfer(out.bytes[1]);
    in.bytes[2] = spi.transfer(out.bytes[2]);
    fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    fastDigitalWrite(PIN_LATCH_IN, LOW);

	uint32_t now = micros();
	debounce_sw01.begin(in.port.sw01, now);
	debounce_sw02.begin(in.port.sw02, now);
	debounce_eject.begin(in.port.sw11, now);
	debounce_insert.begin(in.port.sw12, now);

    // FRAM test, boot count.
    uint32_t bootCount = 0;
    fram.readLong(0x0000, &bootCount);
    Serial.print("boot = ");
    Serial.println(bootCount);
    ++bootCount;
    fram.writeLong(0x0000, bootCount);
}

void loop() {
    uint32_t t1, t2, t3, t4;

    t1 = micros();
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.receive();
    in.bytes[1] = spi.receive();
    in.bytes[2] = spi.receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);
    t2 = micros();

	t3 = micros();
	uint32_t now = micros();
	debounce_sw01.feed(in.port.sw01, now);
	debounce_sw02.feed(in.port.sw02, now);
	debounce_eject.feed(in.port.sw11, now);
	debounce_insert.feed(in.port.sw12, now);
	t4 = micros();

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
		Serial.print(t4 - t3);
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
		Serial.print(t4 - t3);
		Serial.print("us, receive took ");
		Serial.print(t2 - t1);
        Serial.println("us");
    }
}
