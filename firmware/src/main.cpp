#include <Arduino.h>

void setup() {
    // for the code to run, hit C-d in the serial console ti de-activate DTR.
    // this is probably due to layout error.
    Serial.begin(115200);
}

void loop() {
    Serial.println("Hello world!!!!!!!!!");
    delay(1000);
}
