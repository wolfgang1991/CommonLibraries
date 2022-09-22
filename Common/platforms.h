#ifndef PLATFORMS_H_INCLUDED
#define PLATFORMS_H_INCLUDED

//! Preprocessor directives to detect specific platforms

#if defined(__AVR_ARCH__) || defined(ARDUINO) || defined(STM32F1xx)
	#define MICROCONTROLLER_PLATFORM
	#define PLATFORM_STRING "uC"
#elif defined(__APPLE__)
	#if defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
		#define IOS_PLATFORM
		#define PLATFORM_STRING "iOS"
	#else
		#define MACOSX_PLATFORM
		#define PLATFORM_STRING "macOS"
	#endif
#elif defined(__ANDROID__)
	#define ANDROID_PLATFORM
	#define PLATFORM_STRING "Android"
#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	#define WINDOWS_PLATFORM
	#define PLATFORM_STRING "Windows"
#elif defined(__linux__)
	#define LINUX_PLATFORM
	#define PLATFORM_STRING "Linux"
/*#elif defined(__unix__)*/
/*	#define OTHER_UNIX_PLATFORM*/
#else
	#error "No compatible platform detected"
	#define PLATFORM_STRING "Unknown"
#endif


#endif
