#ifndef UCTIMING_H_INCLUDED
#define UCTIMING_H_INCLUDED

#include "platforms.h"

#ifndef MICROCONTROLLER_PLATFORM
#include <timing.h>
//! version of millis() with OS
inline uint32_t millis(){
	return (uint32_t)getMilliSecs();
}
inline uint32_t micros(){
	return (uint32_t)getMicroSecs();
}
typedef uint32_t uc_time_t;
#else
typedef unsigned long uc_time_t;
#endif

//! calculates the difference of timestamps (largeTime-smallTime) and recognizes overflows
//! T: unsigned integral type
template <typename T>
T calcTimeDifference(T largeTime, T smallTime){
	if(largeTime>=smallTime){
		return largeTime-smallTime;
	}else{
		T maxTime = ~static_cast<T>(0);
		return (maxTime-smallTime)+largeTime;
	}
}

#endif
