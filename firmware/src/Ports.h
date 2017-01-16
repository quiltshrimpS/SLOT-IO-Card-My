#ifndef Ports_h
#define Ports_h

#include <Arduino.h>

struct OutPort {
    uint8_t lamp1:1;    // stop 1
    uint8_t lamp2:1;    // stop 2
    uint8_t lamp3:1;    // stop 3
    uint8_t lamp4:1;    // stop 4
    uint8_t lamp5:1;    // stop 5
    uint8_t lamp6:1;    // start
    uint8_t lamp7:1;    // auto
    uint8_t ssr5:1;     // reserved SSR5

    uint8_t lamp8:1;    // max bet
    uint8_t lamp9:1;    // help
    uint8_t lamp10:1;   // extract coin
    uint8_t lamp11:1;   // price 1
    uint8_t lamp12:1;   // price 2
    uint8_t lamp13:1;   // price big
    uint8_t lamp14:1;   // error
    uint8_t ssr6:1;     // reserved SSR6

    uint8_t counter1:1; // score
    uint8_t counter2:1; // wash score (what is this?)
    uint8_t counter3:1; // coin insert
    uint8_t counter4:1; // coin extract
    uint8_t ssr4:1;     // reserved SSR4
    uint8_t ssr3:1;     // reserved SSR3
    uint8_t ssr2:1;     // reserved SSR2
    uint8_t ssr1:1;     // coin extract motor SSR1
};

struct InPort {
    uint8_t sw01:1;     // stop 1
    uint8_t sw02:1;     // stop 2
    uint8_t sw03:1;     // stop 3
    uint8_t sw04:1;     // stop 4
    uint8_t sw05:1;     // stop 5
    uint8_t sw06:1;     // start
    uint8_t sw07:1;     // auto
    uint8_t sw17:1;     // front door

    uint8_t sw08:1;     // max bet
    uint8_t sw09:1;     // help
    uint8_t sw10:1;     // extract coin
    uint8_t sw11:1;     // coin counter
    uint8_t sw12:1;     // coin track 1
    uint8_t sw13:1;     // coin track 2
    uint8_t sw14:1;     // coin track 3
    uint8_t sw20:1;     // banknote

    uint8_t sw15:1;     // open score (??)
    uint8_t sw16:1;     // wash score (??)
    uint8_t sw18:1;     // coin full detect
    uint8_t sw19:1;     // coin full switch
    uint8_t sw21:1;     // S1
    uint8_t sw22:1;     // print S2
    uint8_t sw23:1;     // audit S3
    uint8_t sw24:1;     // settings S4
};

#endif
