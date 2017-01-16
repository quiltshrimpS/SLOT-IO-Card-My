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

WreckedSPI<7, 2, 3, 8> spi;

DigitalPin<4> latchPinOut; // for 74HC595
DigitalPin<9> latchPinIn;  // for 74HC165

union {
    uint8_t bytes[4];
    uint32_t integer;
} data;

uint8_t in[3] = { 0 };

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

    data.integer = 1;
}

void loop() {
    Serial.print(millis());
    Serial.print(": out = ");
    Serial.print((int) (data.bytes[0]), HEX);
    Serial.print(" ");
    Serial.print((int) (data.bytes[1]), HEX);
    Serial.print(" ");
    Serial.print((int) (data.bytes[2]), HEX);
    Serial.print(", in = ");

    uint32_t t1 = micros();
    latchPinOut = LOW;
    latchPinIn = HIGH;
    in[0] = spi.transfer(data.bytes[0]);
    in[1] = spi.transfer(data.bytes[1]);
    in[2] = spi.transfer(data.bytes[2]);
    latchPinOut = HIGH;
    latchPinIn = LOW;
    uint32_t t2 = micros();

    Serial.print((int) (in[0]), HEX);
    Serial.print(" ");
    Serial.print((int) (in[1]), HEX);
    Serial.print(" ");
    Serial.print((int) (in[2]), HEX);
    Serial.print(", took = ");

    Serial.print(t2 - t1);
    Serial.println("us");

    data.integer <<= 1;
    if (data.integer == 1l << 24)
        data.integer = 1;
    delay(500);
}
