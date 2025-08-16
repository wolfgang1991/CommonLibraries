#ifndef STRINGHELPERS_H_INCLUDED
#define STRINGHELPERS_H_INCLUDED

#include "utf8.h"

#include <string>
#include <sstream>
#include <list>
#include <algorithm>
#include <cstdint>
#include <iomanip>

bool isPositiveInteger(const std::string& s);

//! removes spaces at beginning and end
std::string stripSpaces(const std::string s);

std::string convertStringToUpper(const std::string& s);

//! returns filename from a path
std::string stripDir(const std::string& str);

//! returns (path and) filename without extension
std::string stripExt(const std::string& str);

//! returns the file extension from a path/filename without dot (empty string if no extension)
std::string getExt(const std::string& str);

//! returns the path without filename but with trailing slash
std::string stripFile(const std::string& path);

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
bool isStringPrefixEqual(const TString& s, const TString& prefix){
	if(s.size()<prefix.size()){return false;}
	return s.substr(0, prefix.size()).compare(prefix)==0;
}

inline bool isPrefixEqual(const std::string& s, const std::string& prefix){
	return isStringPrefixEqual<std::string>(s, prefix);
}

inline bool isPrefixEqual(const std::wstring& s, const std::wstring& prefix){
	return isStringPrefixEqual<std::wstring>(s, prefix);
}

template <typename TString>
bool isStringSuffixEqual(const TString& s, const TString& suffix){
	if(s.size()<suffix.size()){return false;}
	return s.substr(s.size()-suffix.size(), suffix.size()).compare(suffix)==0;
}

inline bool isSuffixEqual(const std::string& s, const std::string& prefix){
	return isStringSuffixEqual<std::string>(s, prefix);
}

inline bool isSuffixEqual(const std::wstring& s, const std::wstring& prefix){
	return isStringSuffixEqual<std::wstring>(s, prefix);
}

template <typename TString>
TString replaceString(const TString& s, const TString& search, const TString& replace){
	if(search.empty() || search.size()>s.size()){return s;}
	if(s.find(search)==TString::npos){return s;}
	std::basic_stringstream<typename TString::value_type> ss;
	uint32_t i=0;
	uint32_t end = std::max((int32_t)s.size()-(int32_t)search.size()+1,(int32_t)0);
	for(; i<end; i++){
		if(s.substr(i, search.size()).compare(search)==0){
			ss << replace;
			i += search.size()-1;//wegen i++: -1
		}else{
			ss << s[i];
		}
	}
	if(i<s.size()){
		ss << s.substr(i, s.size()-i);
	}
	return ss.str();
}

inline std::string replace(const std::string& s, const std::string& search, const std::string& replace){
	return replaceString<std::string>(s, search, replace);
}

inline std::wstring replace(const std::wstring& s, const std::wstring& search, const std::wstring& replace){
	return replaceString<std::wstring>(s, search, replace);
}

//! special case of parseSeparatedString function template
//! if noEmptyToken: duplicate separator don't result in empty token in list
std::list<std::string> parseSeparatedString(const std::string& s, char separator, bool noEmptyToken = false);

//! parses a separated string e.g. csv and pushed back the token into out. TContainer should be a container where push_back is possible in O(1) (e.g. std::list or std::vector)
//! if noEmptyToken: duplicate separator don't result in empty token in list
template<typename TContainer>
void parseSeparatedString(TContainer& out, const typename TContainer::value_type& s, const typename TContainer::value_type& separators, bool noEmptyToken = false){
	typedef typename TContainer::value_type String;
	typedef typename String::value_type Char;
	std::basic_stringstream<Char> ss;
	for(size_t i=0; i<s.size(); i++){
		Char c = s[i];
		bool isSeparator = false;
		for(size_t i=0; i<separators.size() && !isSeparator; i++){isSeparator = c==separators[i];}
		if(isSeparator){
			out.emplace_back(ss.str());
			if(noEmptyToken && out.back().empty()){out.pop_back();}
			ss.str(String());
		}else{
			ss << c;
		}
	}
	String finalToken = ss.str();
	if(!finalToken.empty()){out.push_back(finalToken);}
}

//! s string to escape
//! escapeCharacter: character used for escaping e.g. '\\'
//! charsToEscape: characters which need to be escaped e.g.: ";,\\\n"
//! charReplacements: characters which shall be inserted after the escape character, e.g. ";,\\n" (same size as charsToEscape)
template<typename TChar>
std::basic_string<TChar> escapeString(const std::basic_string<TChar>& s, const std::basic_string<TChar>& charsToEscape, const std::basic_string<TChar>& charReplacements, TChar escapeCharacter){
	size_t escapeCount = 0;
	for(size_t i=0; i<s.size(); i++){
		TChar c = s[i];
		for(size_t j=0; j<charsToEscape.size(); j++){
			if(c==charsToEscape[j]){
				escapeCount++;
				break;
			}
		}
	}
	if(escapeCount==0){
		return s;
	}else{
		std::basic_string<TChar> res;
		res.reserve(s.size()+escapeCount);
		for(size_t i=0; i<s.size(); i++){
			TChar c = s[i];
			bool notEscaped = true;
			for(size_t j=0; j<charsToEscape.size(); j++){
				if(c==charsToEscape[j]){
					notEscaped = false;
					res.push_back(escapeCharacter);
					res.push_back(charReplacements[j]);
					break;
				}
			}
			if(notEscaped){
				res.push_back(c);
			}
		}
		return res;
	}
}

//! returns a string representation of val rounded to acc decimal places
template<typename TChar>
std::basic_string<TChar> roundToString(double val, int acc){
	std::basic_stringstream<TChar> res;
	res.precision(acc);
	res.setf(std::ios::fixed, std::ios::floatfield);
	res << val;
	return res.str();
}

inline std::string roundToString(double val, int acc){
	return roundToString<char>(val, acc);
}

inline std::string round(double val, int acc){
	return roundToString<char>(val, acc);
}

bool isAnInteger(const std::string& c);

bool isADouble(const std::string& c);

//! space in bytes
template<typename TChar>
std::basic_string<TChar> getHumanReadableSpace(double space, double unit = 1000.0){
	bool lessGB = space<100*1024*1024;
	bool lessMB = space<100*1024;
	double unitSQ = unit*unit;
	std::basic_stringstream<TChar> res;
	res.precision(2);
	res.setf(std::ios::fixed, std::ios::floatfield);
	res << (lessGB?(lessMB?space/unit:space/unitSQ):(space/(unitSQ*unit)));
	res << (TChar)(lessGB?(lessMB?'K':'M'):'G') << (TChar)'B';
	return res.str();
}

inline std::string getHumanReadableSpace(uint64_t space, double unit = 1000.0){
	return getHumanReadableSpace<char>(space, unit);
}

//! removed duplicate /
std::string cleanPath(const std::string& path);

bool isGlobalPath(std::string path);

template<typename TChar>
std::basic_string<TChar> getHumanReadableTime(uint64_t time, bool showDays = true){
	uint64_t secs = time%60;
	uint64_t mins = (time%3600)/60;
	uint64_t hours = (time%86400)/3600;
	uint64_t days = time/86400;
	std::basic_stringstream<TChar> ss;
	if(days>0 && showDays){
		ss << days << (TChar)'d' << (TChar)' ';
	}else{
		hours += days*24;
	}
	ss << std::setw(2) << std::setfill((TChar)'0') << hours << (TChar)':' << std::setw(2) << std::setfill((TChar)'0') << mins << (TChar)':' << std::setw(2) << std::setfill((TChar)'0') << secs;
	return ss.str();
}

//! time in seconds
inline std::string getHumanReadableTime(uint64_t time, bool showDays = true){
	return getHumanReadableTime<char>(time, showDays);
}

//! trims a string (removes whitespaces/control characters from start and end)
template<typename TChar>
std::basic_string<TChar> trimString(const std::basic_string<TChar>& s){
	if(s.empty()){return s;}
	uint32_t start = 0; for(;start<s.size()&&s[start]<=' ';start++){}
	if(start<s.size()){
		uint32_t end = s.size()-1; for(;s[end]<=' ';end--){}
		return s.substr(start, end-start+1);
	}else{//only whitespaces etc
		return std::basic_string<TChar>();
	}
}

//! returns the path prefix (including trailing slash) to access file in the same folder as the executable
std::string getAppHomePathFromArgV0(const char* argv0);

//! returns true if it is there, and fills the value (empty==no value, defaultValue if argument doesn't exist), examples: .. -arg .. , .. -arg value .. , .. -argvalue ..
bool getCommandLineArgument(std::string& value, int argc, char* argv[], const std::string& argument, const std::string& defaultValue = "", char argPrefix = '-');

template <typename TValue>
bool getCommandLineArgument(TValue& value, int argc, char* argv[], const std::string& argument, const TValue& defaultValue = TValue(), char argPrefix = '-'){
	std::string strValue;
	bool res = getCommandLineArgument(strValue, argc, argv, argument, "", argPrefix);
	value = res?convertStringTo<TValue>(strValue):defaultValue;
	return res;
}

//! only checks for existance without value
bool hasCommandLineArgument(int argc, char* argv[], const char* argument);

#endif
