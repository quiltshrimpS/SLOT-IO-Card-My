#ifndef __UTIL_H__
#define __UTIL_H__

#include <Arduino.h>
#include "Configuration.h"

// likely / unlikely stolen from linux kernel
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

// counter check
void badCounter() __attribute__((error("counter index is not a constant, and this only applys to counter 0 ~ 3")));

static inline __attribute__((always_inline))
void badCounterCheck(uint8_t counter) {
  if (!__builtin_constant_p(counter) || (counter >= 4 && counter != COUNTER_NOT_A_COUNTER)) {
     badCounter();
  }
}

#endif
