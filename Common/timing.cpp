#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <timing.h>

static uint64_t initSecs(){
	#if _WIN32
	return timeGetTime()/1000;
	#else
	struct timespec tc;
	clock_gettime(CLOCK_MONOTONIC, &tc);
	return (uint64_t)(tc.tv_sec);
	#endif
}

uint64_t offSeconds = initSecs();

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

void delay(uint32_t ms){
	#if _WIN32
	Sleep(ms);
	#else
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
	#endif
}

int64_t getEpochSecs(){
	return (int64_t)time(NULL);
}
