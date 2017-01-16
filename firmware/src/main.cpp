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

DigitalPin<3> clockPin;
DigitalPin<2> dataPin;
DigitalPin<4> latchPin;

union {
    uint8_t bytes[4];
    uint32_t integer;
} data;

void setup() {
    // for the code to run, hit C-d in the serial console ti de-activate DTR.
    // this is probably due to layout error.
    Serial.begin(115200);

    pinMode(5, OUTPUT); // 595_nCLR
    pinMode(6, OUTPUT); // 595_nG

    digitalWrite(5, LOW); // clear 74HC595 memories (toggle it on lator)
    digitalWrite(6, LOW); // enable 74HC595
    latchPin.config(OUTPUT, HIGH); // latchPin in output mode with initial level HIGH.
    clockPin.config(OUTPUT, LOW); // clockPin in output mode with initial level LOW.
    dataPin.config(OUTPUT, HIGH); // dataPin in output mode with initial level HIGH.
    digitalWrite(5, HIGH); // cleared 74HC595, put it back on.

    data.integer = 1;
}

//------------------------------------------------------------------------------
// Time to send one bit is ten cycles or 625 ns for 16 MHz CPU.
inline __attribute__((always_inline))
void sendBit(uint8_t bit, uint8_t data) {
  dataPin = data & (1 << bit);
  clockPin = 1;
  // may want a nop here - clock pulse is 125 ns wide
  clockPin = 0;
}
//------------------------------------------------------------------------------
// Time to send one byte is 5 usec.
void shiftOut(uint8_t bits) {
  sendBit(7, bits);
  sendBit(6, bits);
  sendBit(5, bits);
  sendBit(4, bits);
  sendBit(3, bits);
  sendBit(2, bits);
  sendBit(1, bits);
  sendBit(0, bits);
}

void loop() {
    Serial.print(millis());
    Serial.print(": ");
    Serial.print((int) (data.bytes[0]), HEX);
    Serial.print(" ");
    Serial.print((int) (data.bytes[1]), HEX);
    Serial.print(" ");
    Serial.print((int) (data.bytes[2]), HEX);
    Serial.print(" ");
    Serial.print((int) (data.bytes[3]), HEX);
    Serial.print(", took = ");

    uint32_t t1 = micros();
    latchPin = LOW;
    shiftOut(data.bytes[0]);
    shiftOut(data.bytes[1]);
    shiftOut(data.bytes[2]);
    latchPin = HIGH;
    uint32_t t2 = micros();

    Serial.print(t2 - t1);
    Serial.println("us");

    data.integer <<= 1;
    if (data.integer == 1l << 24)
        data.integer = 1;
    delay(500);
}
