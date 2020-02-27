#ifndef INIPARSER_H_INCLUDED
#define INIPARSER_H_INCLUDED

#include <string>

//! Parser for inis via a DFA
class IniParser {

	private:
	
	int state;
	
	public:
	
	std::string curSection;
	std::string curKey;
	std::string curValue;

	IniParser();

	//! Returns true if DFA has recognized a key value pair
	//! Usage: Continuously feed doStep with characters and use key value pair if doStep returns true.
	//! In case of EOF call doStep with a '\n' to finish parsing the last line.
	bool doStep(char c);
};

#endif
