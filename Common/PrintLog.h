#ifndef PrintLog_H_
#define PrintLog_H_

//! Simple logging abstraction for different platforms

#include <string>
#include <sstream>

//! same as android log priority
enum LogPriority{
	LOG_UNKNOWN, LOG_DEFAULT, LOG_VERBOSE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL, LOG_SILENT, LOG_COUNT
};

//! tag is disregarded if the underlying logging system does not support this (e.g. std::cout)
//! std::cout if prio <= LOG_INFO and std::cerr if prio >= LOG_WARN, or depending on underlying logging system
void printLog(const std::string& message, int prio = LOG_INFO, const char* tag = NULL);

template<typename A, typename B>
void printLog2(const A& a, const B& b, int prio = LOG_INFO, const char* tag = NULL){
	std::stringstream ss; ss << a << b;
	printLog(ss.str(), prio, tag);
}

#endif
