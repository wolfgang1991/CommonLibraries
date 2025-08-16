#ifndef uCProfiler_H_
#define uCProfiler_H_

// Profiling Helper Macros (see AuxBoxFirmware.ino and SensorBoxUSB.ino for usage example)
// Designed for usage with uCRPC.
// To enable define USE_PROFILING before including this file
// ApiString and printDebug function implementations are required
// Possible implementations:
//using ApiString = UCRPC::String<256>;
//template<typename TRPC>
//void printDebug(TRPC* rpc, const ApiString& data, UCRPC::IUCRemoteProcedureCaller* caller = nullptr){
//	rpc->callRemoteProcedure(PRINT_DEBUG, data, caller);
//}

#ifdef USE_PROFILING
#define PROFILE(P) P
#else
#define PROFILE(P)
#endif

#ifndef PROFILE_MSG_PREFIX
#define PROFILE_MSG_PREFIX "-"
#endif

//! Global variables should be put globally before setup() and loop() functions
#define PROFILE_GLOBAL PROFILE(uc_time_t lastTime; \
float averageDt = 0; \
uint32_t averageDtCount = 0; \
uc_time_t lastPrintTime = 0; \
uc_time_t t_profile; \
uc_time_t stepDt; \
uc_time_t ta; )

//! Needs to be put somewhere in the setup function
#define PROFILE_SETUP PROFILE(lastTime = millis();)

//! Needs to be put at the beginning of the loop function
#define PROFILE_LOOP PROFILE(ta = micros(); \
	uc_time_t dt = calcTimeDifference(ta, lastTime); \
	uint32_t averageDtCountP1 = averageDtCount + 1; \
	averageDt = averageDt*((float)averageDtCount/(float)averageDtCountP1) + ((float)dt)/1000.f*(1.f/(float)averageDtCountP1); \
	averageDtCount = averageDtCountP1; \
	if(calcTimeDifference(millis(), lastPrintTime)>1000){ \
		printDebug(&ucrpc, ApiString(PROFILE_MSG_PREFIX " averageDt [µs]: ") << (int)(averageDt*1000.f) << " averageDtCount: " << averageDtCount); \
		lastPrintTime = millis(); \
	} \
	ta = lastTime = micros();)

//! Should be put after each step which shall be measured, NAME: string fpr debug messages, USLIMIT limit in µs after which a debug message is sent
#define PROFILE_STEP(NAME, USLIMIT) PROFILE( \
	t_profile = micros(); \
	stepDt = calcTimeDifference(t_profile, ta); \
	if(stepDt>USLIMIT){printDebug(&ucrpc, ApiString(PROFILE_MSG_PREFIX " " NAME) << stepDt);} \
	ta = micros();)


#endif
