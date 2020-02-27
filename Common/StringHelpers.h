#ifndef STRINGHELPERS_H_INCLUDED
#define STRINGHELPERS_H_INCLUDED

#include <string>
#include <sstream>
#include <list>

//! Convert some value to a string
template <typename T>
inline std::string convertToString(T Number){
	std::stringstream ss;
	ss << Number;
	return ss.str();
}

//! Convert a wstring to some value
template <typename T>
inline T convertWStringTo(const std::wstring& Str){
	std::wstringstream ss(Str);
	T Number = 0;
	ss >> Number;
	return Number;
}

//! Convert a string to some value
template <typename T>
inline T convertStringTo(const std::string& Str){
	std::stringstream ss(Str);
	T Number = 0;
	ss >> Number;
	return Number;
}

//! Convert some value to a string
template <typename T>
inline std::wstring convertToWString(T Number){
	std::wstringstream ss;
	ss << Number;
	return ss.str();
}

//! Check if prefix of s matches
template <typename TString>
bool isPrefixEqual(const TString& s, const TString& prefix){
	if(s.size()<prefix.size()){return false;}
	return s.substr(0, prefix.size()).compare(prefix)==0;
}

inline bool isPrefixEqual(const std::string& s, const std::string& prefix){
	return isPrefixEqual<std::string>(s, prefix);
}

inline bool isPrefixEqual(const std::wstring& s, const std::wstring& prefix){
	return isPrefixEqual<std::wstring>(s, prefix);
}

//! Convert String to WString
std::wstring convertStringToWString(const std::string& str);

//! Convert WString to String
std::string convertWStringToString(const std::wstring& str);

std::list<std::string> parseSeparatedString(const std::string& s, char seperator);

//! returns a string representation of val rounded to acc decimal places
std::string round(double val, int acc);

bool isAnInteger(const std::string& c);

//! space in bytes
std::string getHumanReadableSpace(uint64_t space);

//! time in seconds
std::string getHumanReadableTime(uint64_t time, bool showDays = true);

#endif
