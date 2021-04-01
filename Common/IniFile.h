#ifndef INIFILE_H_INCLUDED
#define INIFILE_H_INCLUDED

#include "IniIterator.h"

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

	void setFromString(const char* str);

	std::string toString() const;

	IniFile(const std::string& file);

	//! get value (creates empty string for value if not available)
	std::string& get(const std::string& section, const std::string& key);
	
	const std::string& get(const std::string& section, const std::string& key) const;
	
	const std::string& get(const std::string& section, const std::string& key, const std::string& defaultValue) const;

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
	
};

#endif
