#ifndef TIMING_H_INCLUDED
#define TIMING_H_INCLUDED

#include <stdint.h>

extern uint64_t offSeconds;

//! return time in milliseconds since program start
uint64_t getMilliSecs();

//! wait for some time in milliseconds, 0-2^32
void delay(uint32_t ms);

//! useful for gui/highlighting etc (returns time dependent value between 0-1)
double blink();

//! Time in seconds since program start
double getSecs();

//! UTC seconds since 1.1.1970
int64_t getEpochSecs();

#endif
