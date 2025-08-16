#ifndef XMLParser_H_INCLUDED
#define XMLParser_H_INCLUDED

#include <string>
#include <unordered_map>
#include <sstream>

inline bool isWhitespace(char c){
	return c==' ' || c=='\t' || c=='\n';
}

//! escapes &, <, >, ", and '
std::string escapeXMLString(const std::string& s);

//! removes escapes from escapeXMLString
std::string unescapeXMLString(const std::string& s);

class XMLTag{

	public:

	XMLTag(XMLTag* parent);

	XMLTag* parent;//! Null, falls kein Parent
	std::string name;
	std::unordered_map<std::string, std::string> attributes;//unordered_map faster than map for strings
	std::stringstream intermediate;

	std::string getInheritance();

	void printInheritance();

};

class XMLParser;

class IParsingCallback{

	public:

	virtual void OnOpenTag(XMLParser* p) = 0;

	virtual void OnCloseTag(XMLParser* p) = 0;

	virtual void OnFinishFile(XMLParser* p){}

	virtual void OnFinishAll(XMLParser* p){}

	virtual ~IParsingCallback(){}

};

//! Decides wheter an intermediate character (between open and close tag) shall be used. (E.g. many formats want to disregard newlines.)
class IIntermediateCharacterFilter{
	
	public:

	virtual bool useIntermediateChar(char c){
		return c!='\n' && c!='\r';
	}
	
	virtual ~IIntermediateCharacterFilter(){}

};

class FastReuseStringStream{

	std::string s;

	public:
	
	FastReuseStringStream& operator<<(char c){
		s.push_back(c);//amortized constant
		return *this;
	}
	
	const std::string& str(){
		return s;
	}
	
	void reset(){
		s.clear();//existing implementations don't change the capacity, although not guaranteed by standard
	}
	
};

class XMLParser{

	private:

	XMLTag* dom;

	int mode;//state of the stack machine

	FastReuseStringStream token;
	//std::string ckey;
	std::string* valueDestination;

	IParsingCallback* cbk;

	int line;

	void domBack();
	
	IIntermediateCharacterFilter* charFilter;

	public:

	XMLParser(IParsingCallback* callback);
	
	~XMLParser();
	
	//! charFilter will be deleted
	void setIntermediateCharacterFilter(IIntermediateCharacterFilter* charFilter);

	//! parse nect character
	void parse(char c);

	//! The current/intermediate Document Object Model represents the stack of the stack machine, the returned XMLTag is the tag which is parsed most recently
	XMLTag* getCurrentDOM();

	int getCurrentLine();

	void OnFinishFile();

	void OnFinishAll();

};

void parseFile(XMLParser* parser, std::string file);

#endif
