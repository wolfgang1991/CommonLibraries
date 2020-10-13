#ifndef STRINGHELPERS_H_INCLUDED
#define STRINGHELPERS_H_INCLUDED

#include <string>
#include <sstream>
#include <list>

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

//! Convert String to WString
std::wstring convertStringToWString(const std::string& str);


//! Convert WString to String
std::string convertWStringToString(const std::wstring& str);

//! special case of parseSeparatedString function template
std::list<std::string> parseSeparatedString(const std::string& s, char separator);

//! parses a separated string e.g. csv and pushed back the token into out. TContainer should be a container where push_back is possible in O(1) (e.g. std::list or std::vector)
template<typename TContainer>
void parseSeparatedString(TContainer& out, const typename TContainer::value_type& s, typename TContainer::value_type::value_type separator){
	typedef typename TContainer::value_type String;
	typedef typename String::value_type Char;
	std::basic_stringstream<Char> ss;
	for(size_t i=0; i<s.size(); i++){
		Char c = s[i];
		if(c==separator){
			out.push_back(ss.str());
			ss.str(String());
		}else{
			ss << c;
		}
	}
	String finalToken = ss.str();
	if(!finalToken.empty()){out.push_back(finalToken);}
}

//! returns a string representation of val rounded to acc decimal places
std::string round(double val, int acc);

bool isAnInteger(const std::string& c);

//! space in bytes
std::string getHumanReadableSpace(uint64_t space);

//! time in seconds
std::string getHumanReadableTime(uint64_t time, bool showDays = true);

bool isGlobalPath(std::string path);

//! returns the path prefix (including trailing slash) to access file in the same folder as the executable
std::string getAppHomePathFromArgV0(const char* argv0);

#endif
