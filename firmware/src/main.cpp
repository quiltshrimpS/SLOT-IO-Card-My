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

FRAM_MB85RC_I2C fram(MB85RC_ADDRESS_A000, true, A7, 16 /* kb */);
WreckedSPI<7, 2, 8, 3> spi;

DigitalPin<4> latchPinOut; // for 74HC595
DigitalPin<9> latchPinIn;  // for 74HC165

union {
    uint8_t bytes[4];
    uint32_t integer;
} out;

uint8_t in[3] = { 0 };
uint8_t cur_in[3] = { 0 };

uint32_t last_diag_millis;

void setup() {
    Serial.begin(115200);

    pinMode(5, OUTPUT); // 595_nCLR
    pinMode(6, OUTPUT); // 595_nG
    digitalWrite(5, LOW); // clear 74HC595 memories (toggle it on lator)
    digitalWrite(6, LOW); // enable 74HC595

    latchPinOut.config(OUTPUT, HIGH); // latchPinOUT in output with initial HIGH
    latchPinIn.config(OUTPUT, LOW); // latchPinIn in input with initial LOW
    spi.begin(); // for 74HC595 and 74HC165
    Wire.begin(); // for FRAM
    fram.begin();

    out.integer = 1;
    last_diag_millis = millis();

    // cleared 74HC595, put it back on.
    // (do this as the last tin in `setup()`)
    digitalWrite(5, HIGH);

    uint32_t bootCount = 0;
    fram.readLong(0x0000, &bootCount);
    Serial.print("boot = ");
    Serial.println(bootCount);
    ++bootCount;
    fram.writeLong(0x0000, bootCount);
}

uint32_t ejected = 0;

void loop() {
    uint32_t t1 = micros();
    latchPinOut = LOW;
    latchPinIn = HIGH;
    in[0] = spi.transfer(out.bytes[0]);
    in[1] = spi.transfer(out.bytes[1]);
    in[2] = spi.transfer(out.bytes[2]);
    latchPinOut = HIGH;
    latchPinIn = LOW;

    bool do_print = false;
    if (in[0] != cur_in[0] || in[1] != cur_in[1] || in[2] != cur_in[2]) {
        if (~in[1] & 0x08)
            ++ejected;

        cur_in[0] = in[0];
        cur_in[1] = in[1];
        cur_in[2] = in[2];
        do_print = true;
    }
    uint32_t t2 = micros();

    if (do_print) {
        Serial.print(millis());
        Serial.print(": in = ");
        Serial.print((int) (in[0]), HEX);
        Serial.print(" ");
        Serial.print((int) (in[1]), HEX);
        Serial.print(" ");
        Serial.print((int) (in[2]), HEX);
        Serial.print(", ejected = ");
        Serial.print(ejected);
        Serial.print(", took = ");
        Serial.print(t2 - t1);
        Serial.println("us");
    }

    if (millis() - last_diag_millis > 500) {
        last_diag_millis = millis();
        out.integer <<= 1;
        if (out.integer == 1l << 24)
            out.integer = 1;

        Serial.print(millis());
        Serial.print(": out = ");
        Serial.print((int) (out.bytes[0]), HEX);
        Serial.print(" ");
        Serial.print((int) (out.bytes[1]), HEX);
        Serial.print(" ");
        Serial.print((int) (out.bytes[2]), HEX);
        Serial.print(", took = ");
        Serial.print(t2 - t1);
        Serial.println("us");
    }
}
