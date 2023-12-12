#ifndef INIFILE_H_INCLUDED
#define INIFILE_H_INCLUDED

#include "IniIterator.h"
#include "StringHelpers.h"

//! '\n' => \n   \ => \\   # => \#   ; => \;   ' ' => \' '   '\t' => \'\t'   [ => \0   ] => \1    = => \2
std::string escapeIniStrings(const std::string& s);

//! Reverse function of escapeIniStrings
std::string unescapeIniStrings(const std::string& s);

//! Load/Save Ini Files etc
//! Syntax:
//! Ini = (W*(C|S|P)W*'\n')*
//! W = ' '|'\t' (whitespace)
//! C = (';'|'#')Sigma* (comments)
//! S = '['Sigma*']'Sigma* (sections)
//! P = KW*'='W*K (key-value pairs)
//! K = Sigma*(Sigma\W) (key or value)
//! Sigma = Alphabet (extension of ASCII with character length 1 byte without '\n' and without '\0')
class IniFile{
	public:
	std::map< std::string, std::map<std::string, std::string> > data;

	IniFile(){}

	//! sets new content from string without erasing the old content (merge)
	void setFromString(const char* str);
	
	std::string toString() const;

	IniFile(const std::string& file);
	
	const std::string& get(const std::string& section, const std::string& key) const;
	
	const std::string& get(const std::string& section, const std::string& key, const std::string& defaultValue) const;
	
	static const std::string& get(std::map< std::string, std::map<std::string, std::string> >::iterator it, const std::string& key, const std::string& defaultValue);

	//! set value
	void set(const std::string& section, const std::string& key, const std::string& value);

	//! remove section with all key value pairs
	void removeSection(const std::string& section);

	//! remove key and value
	void removeValue(const std::string& section, const std::string& key);

	//! returns new pointer to iterator object
	IniIterator* createNewIterator() const;

	void print() const;

	bool save(const std::string& file) const;

	//! true if available
	bool isAvailable(const std::string& section, const std::string& key) const;

	//! true if available
	bool isSectionAvailable(const std::string& section) const;

	//! Moves a section's (start) keys and values to another section (end).
	void moveSection(const std::string& start, const std::string& end);
	
	//! returns an entire section if available, otherwise empty map
	const std::map<std::string, std::string>& getSection(const std::string& section) const;
	
	void setSection(const std::string& section, const std::map<std::string, std::string>& key2value);
	
};

// Functions to simplify writeToIni and readToIni functions, see IrrlichtExtensions/AppTracker as an example

#define INI_SELECT_CONST_SECTION(INI, SECTION) \
	static const std::map<std::string, std::string> sectionFallback; \
	auto it = (INI).data.find(SECTION); \
	const std::map<std::string, std::string>& sectionReference = it==(INI).data.end()?sectionFallback:(it->second);

#define INI_GET(VARIABLE, DEFAULT_VALUE) \
	VARIABLE = getFromIniSection(sectionReference, #VARIABLE, DEFAULT_VALUE);
	
#define INI_GET_VECTOR(VECTOR) \
	{ \
		uint32_t size = getFromIniSection(sectionReference, #VECTOR".size", (uint32_t)0); \
		VECTOR.clear(); \
		VECTOR.reserve(size); \
		for(uint32_t i=0; i<size; i++){ \
			std::stringstream ss; ss << #VECTOR"[" << i << "]"; \
			VECTOR.emplace_back(getFromIniSection(sectionReference, ss.str(), decltype(VECTOR)::value_type())); \
		} \
	}

template <typename TValue, typename std::enable_if<!std::is_same<TValue,std::string>::value && !std::is_same<TValue,std::wstring>::value && !std::is_same<TValue,bool>::value, int>::type = 0>
TValue getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, TValue defaultValue){
	auto it = section.find(key);
	if(it == section.end()){
		return defaultValue;
	}else{
		return convertStringTo<TValue>(it->second);
	}
}

const std::string& getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, const std::string& defaultValue);

std::wstring getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, const std::wstring& defaultValue);

bool getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, bool defaultValue);

#define INI_SELECT_SECTION(INI, SECTION) \
	std::map<std::string, std::string>& sectionReference = (INI).data[SECTION];

#define INI_SET(VARIABLE) \
	setToIniSection(sectionReference, #VARIABLE, VARIABLE);
	
#define INI_SET_KV(KEY, VALUE) \
	setToIniSection(sectionReference, KEY, VALUE);

#define INI_SET_VECTOR(VECTOR) \
	INI_SET_KV(#VECTOR".size", VECTOR.size()); \
	for(uint32_t i=0; i<VECTOR.size(); i++){ \
		std::stringstream ss; ss << #VECTOR"[" << i << "]"; \
		INI_SET_KV(ss.str(), VECTOR[i]) \
	}

template <typename TValue, typename std::enable_if<!std::is_same<TValue,std::string>::value && !std::is_same<TValue,std::wstring>::value && !std::is_same<TValue,bool>::value, int>::type = 0>
void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, TValue value){
	section[key] = convertToString<TValue>(value);
}

void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, const std::string& value);

void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, const std::wstring& value);

void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, bool value);

#endif
