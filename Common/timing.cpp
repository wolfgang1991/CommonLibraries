#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "timing.h"

#if _WIN32
#warning "better implementation for windows required"
#endif

static uint64_t initSecs(){
	#if _WIN32
	return timeGetTime()/1000;
	#else
	struct timespec tc;
	clock_gettime(CLOCK_MONOTONIC, &tc);
	return (uint64_t)(tc.tv_sec);
	#endif
}

static const uint64_t offSeconds = initSecs();

double blink(){
	return fabs(sin(((getMilliSecs()/5) % 360) * (3.14/180.0)));
}

double getSecs(){
	#if _WIN32
	return ((double)timeGetTime())/1000.0;
	#else
	struct timespec tc;
	clock_gettime(CLOCK_MONOTONIC, &tc);
	return (double)(tc.tv_sec-offSeconds) + ((double)(tc.tv_nsec)/1000000000.0);
	#endif
}

uint64_t getMilliSecs(){
	#if _WIN32
	return timeGetTime();
	#else
	struct timespec tc;
	clock_gettime(CLOCK_MONOTONIC, &tc);
	return ((uint64_t)(tc.tv_sec-offSeconds))*1000 + (uint64_t)(tc.tv_nsec/1000000);
	#endif
}

uint64_t getMicroSecs(){
	#if _WIN32
	return getMilliSecs()*1000;
	#else
	struct timespec tc; clock_gettime(CLOCK_MONOTONIC, &tc);
	return (uint64_t)(tc.tv_sec-offSeconds)*1000000 + (uint64_t)(tc.tv_nsec/1000);//overflow ok
	#endif
}

void delay(uint32_t ms){
	#if _WIN32
	Sleep(ms);
	#else
	usleep(ms*1000);
	#endif
}

void delayMicroseconds(uint32_t us){
	#if _WIN32
	delay(us/1000);
	#warning "Proper implementation of delayMicroseconds missing"
	#else
	usleep(us);
	#endif
}

int64_t getEpochSecs(){
	return (int64_t)time(NULL);
}
