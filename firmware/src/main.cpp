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

#include "WreckedSPI.h"
#include "Ports.h"

FRAM_MB85RC_I2C fram(MB85RC_ADDRESS_A000, true, /* WP */ A7, 16 /* kb */);
WreckedSPI< /* MISO */ 7, /* MOSI */ 2, /* SCLK_MISO */ 8, /* SCLK_MOSI */ 3, /* MODE_MISO */ 2, /* MODE_MOSI */ 0 > spi;

#define COIN_EJECT_LEVEL                (HIGH)
#define COIN_EJECT_DEBOUNCE_TIMEOUT_US  (90000)
#define COIN_INSERT_DEBOUNCE_TIMEOUT_US (90000)

static uint8_t const PIN_LATCH_OUT = 4; // for 74HC595
static uint8_t const PIN_LATCH_IN = 9;  // for 74HC165

union {
    uint8_t bytes[3];
    struct OutPort port;
} out;

union {
    uint8_t bytes[3];
    struct InPort port;
} in, cur_in;

static uint32_t last_coin_eject_micros;
static uint32_t last_coin_insert_micros;

void setup() {
    fastPinConfig(5, OUTPUT, LOW); // 595 nCLR (active LOW), clear register
    fastPinConfig(6, OUTPUT, LOW); // 595 nG (active LOW), enable
    fastPinConfig(PIN_LATCH_OUT, OUTPUT, HIGH); // 595 latch
    fastPinConfig(PIN_LATCH_IN, OUTPUT, LOW);   // 165 latch

    Serial.begin(115200);

    spi.begin(); // for 74HC595 and 74HC165

    Wire.begin(); // for FRAM
    fram.begin();

    last_coin_eject_micros = last_coin_insert_micros = micros();

    // cleared 74HC595, put it back on.
    fastDigitalWrite(5, HIGH);

    // read the initial states, and write the initial states.
    fastDigitalWrite(PIN_LATCH_OUT, LOW);
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    cur_in.bytes[0] = spi.transfer(out.bytes[0]);
    cur_in.bytes[1] = spi.transfer(out.bytes[1]);
    cur_in.bytes[2] = spi.transfer(out.bytes[2]);
    fastDigitalWrite(PIN_LATCH_OUT, HIGH);
    fastDigitalWrite(PIN_LATCH_IN, LOW);

    // FRAM test, boot count.
    uint32_t bootCount = 0;
    fram.readLong(0x0000, &bootCount);
    Serial.print("boot = ");
    Serial.println(bootCount);
    ++bootCount;
    fram.writeLong(0x0000, bootCount);
}

int32_t inserted = 0;
int32_t ejected = 0;

uint32_t p_0 = 0, p_1 = 0;

void loop() {
    uint32_t t1, t2, t3 = 0, t4 = 0;

    t1 = micros();
    fastDigitalWrite(PIN_LATCH_IN, HIGH);
    in.bytes[0] = spi.receive();
    in.bytes[1] = spi.receive();
    in.bytes[2] = spi.receive();
    fastDigitalWrite(PIN_LATCH_IN, LOW);
    t2 = micros();

    bool do_print = false, do_send = false;
    if (in.bytes[0] != cur_in.bytes[0] || in.bytes[1] != cur_in.bytes[1] || in.bytes[2] != cur_in.bytes[2]) {
        // coin eject
        if (in.port.sw11 != cur_in.port.sw11) {
            if (in.port.sw11 == COIN_EJECT_LEVEL) {
                p_1 = micros();
            } else {
                if (p_0 - p_1 > 10000) {
                    if (micros() - last_coin_eject_micros > COIN_EJECT_DEBOUNCE_TIMEOUT_US) {
                        last_coin_eject_micros = micros();
                        ++ejected;
                    }
                }
                p_0 = micros();
            }
        }

        if (in.port.sw12 != cur_in.port.sw12) {
            if (in.port.sw12 == LOW)
                if (micros() - last_coin_insert_micros > COIN_INSERT_DEBOUNCE_TIMEOUT_US) {
                    last_coin_insert_micros = micros();
                    ++inserted;
                }
        }

        if (in.port.sw01 != cur_in.port.sw01) {
            out.port.ssr5 = !in.port.sw01;
            do_send = true;
        }

        if (in.port.sw02 != cur_in.port.sw02) {
            if (in.port.sw02 == LOW) {
                ejected = 0;
                inserted = 0;
            }
        }

        cur_in.bytes[0] = in.bytes[0];
        cur_in.bytes[1] = in.bytes[1];
        cur_in.bytes[2] = in.bytes[2];
        do_print = true;
    }

    if (do_send) {
        t3 = micros();
        fastDigitalWrite(PIN_LATCH_OUT, LOW);
        spi.send(out.bytes[0]);
        spi.send(out.bytes[1]);
        spi.send(out.bytes[2]);
        fastDigitalWrite(PIN_LATCH_OUT, HIGH);
        t4 = micros();

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
        Serial.print(", took = ");
        Serial.print(t4 - t3);
        Serial.println("us");
    }

    if (do_print) {
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
        if (in.port.sw11 != COIN_EJECT_LEVEL) {
            Serial.print(", period = ");
            Serial.print(p_0 - p_1);
            if (p_0 - p_1 < 10000)
                Serial.print(" (X)");
        }
        Serial.print(", took = ");
        Serial.print(t2 - t1);
        Serial.println("us");
    }
}
