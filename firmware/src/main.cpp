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
 */

#include <Arduino.h>
#include <DigitalIO.h>

#include "WreckedSPI.h"

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
    // for the code to run, hit C-d in the serial console ti de-activate DTR.
    // this is probably due to layout error.
    Serial.begin(115200);

    pinMode(5, OUTPUT); // 595_nCLR
    pinMode(6, OUTPUT); // 595_nG

    digitalWrite(5, LOW); // clear 74HC595 memories (toggle it on lator)
    digitalWrite(6, LOW); // enable 74HC595
    latchPinOut.config(OUTPUT, HIGH); // latchPinOUT in output with initial HIGH
    latchPinIn.config(OUTPUT, LOW); // latchPinIn in input with initial LOW
    spi.begin();
    digitalWrite(5, HIGH); // cleared 74HC595, put it back on.

    out.integer = 1;

    last_diag_millis = millis();
}

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
        cur_in[0] = in[0];
        cur_in[1] = in[1];
        cur_in[2] = in[2];
        do_print = true;
    }
    uint32_t t2 = micros();

    delayMicroseconds(50);

    if (do_print) {
        Serial.print(millis());
        Serial.print(": in = ");
        Serial.print((int) (in[0]), HEX);
        Serial.print(" ");
        Serial.print((int) (in[1]), HEX);
        Serial.print(" ");
        Serial.print((int) (in[2]), HEX);
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
