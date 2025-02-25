#include "PrintLog.h"

#include <platforms.h>

#ifdef ANDROID_PLATFORM
#include <android/log.h>
#else
#include <iostream>
#endif

void printLog(const std::string& message, int prio, const char* tag){
	#ifdef ANDROID_PLATFORM
	__android_log_print(prio, tag, "%s", message.c_str());
	#else
	if(prio<LOG_INFO){
		std::cout << message << std::endl;
	}else{
		std::cerr << message << std::endl;
	}
	#endif
}
