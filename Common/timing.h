#ifndef TIMING_H_INCLUDED
#define TIMING_H_INCLUDED

#include <stdint.h>

//! return time in milliseconds since program start
uint64_t getMilliSecs();

uint64_t getMicroSecs();

//! wait for some time in milliseconds, 0-2^32
void delay(uint32_t ms);

//! like delay mit microseconds
void delayMicroseconds(uint32_t us);

//! useful for gui/highlighting etc (returns time dependent value between 0-1)
double blink();

//! Time in seconds since program start
double getSecs();

//! UTC seconds since 1.1.1970
int64_t getEpochSecs();

#endif
