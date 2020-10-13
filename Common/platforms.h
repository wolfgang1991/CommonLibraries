#ifndef PLATFORMS_H_INCLUDED
#define PLATFORMS_H_INCLUDED

//! Preprocessor directives to detect specific platforms

#if defined(__AVR_ARCH__) || defined(ARDUINO)
	#define MICROCONTROLLER_PLATFORM
#elif defined(__APPLE__)
	#if defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
		#define IOS_PLATFORM
	#else
		#define MACOSX_PLATFORM
	#endif
#elif defined(__ANDROID__)
	#define ANDROID_PLATFORM
#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	#define WINDOWS_PLATFORM
#elif defined(__linux__)
	#define LINUX_PLATFORM
/*#elif defined(__unix__)*/
/*	#define OTHER_UNIX_PLATFORM*/
#else
	#error "No compatible platform detected"
#endif


#endif
