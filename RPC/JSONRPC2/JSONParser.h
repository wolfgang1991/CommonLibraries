#ifndef JSONParser_H_INCLUDED
#define JSONParser_H_INCLUDED

#include <IRPC.h>

//! escapeNonPrintableChars: if true it is standard compliant, however it works with this parser also if they are not escaped (==false, more efficient in case binary data is sent as strings)
//! floatingPointPrecision: digits used / 0 means default
std::string convertRPCValueToJSONString(const IRPCValue& value, bool escapeNonPrintableChars, uint8_t floatingPointPrecision = 0);

//! escapeNonPrintableChars: if true it is standard compliant, however it works with this parser also if they are not escaped (==false, more efficient in case binary data is sent as strings)
std::string escapeAndQuoteJSONString(const std::string& s, bool escapeNonPrintableChars);

//! Interface for JSON Parsers
//! The parsed value is represented as RPC value. Therefore the parser is especially meaningful in an RPC Context (e.g. JSON RPC 2).
class IJSONParser{
	
	public:
	
	enum State{
		SUCCESS,
		PARSING,
		ERROR,
		STATE_COUNT
	};
	
	//! Reset internal parser state to accept new input
	virtual void reset() = 0;
	
	//! Parses a new character (works with UTF-8 and ascii compatible character sets, WARNING: \uXXXX is converted into an utf8 value)
	//! The lookahead must be '\0' if the input has ended (no further characters will follow). It is treated as a whitespace.
	virtual State parse(char c, char lookahead) = 0;
	
	//! Returns the result which is removed from the parser. Returns NULL if there is no result.
	//! If a result has not been stolen it will be deleted on reset or destuction.
	virtual IRPCValue* stealResult() = 0;
	
	//! Returns NULL if there is no result.
	virtual IRPCValue* getResult() = 0;
	
	virtual ~IJSONParser(){}
	
};

class JSONStringParser;
class JSONSpecialTokenParser;
class JSONNumberParser;
class JSONArrayParser;
class JSONObjectParser;

//! A JSON Parser consists of different sub-parsers for different JSON sub sets
//! It effectively parses a "value" as defined in https://www.json.org/json-en.html
class JSONParser : public IJSONParser{
	
	protected:
	
	std::vector<IJSONParser*> subParser;
	
	int state;
	
	public:
	
	JSONParser();
	
	virtual ~JSONParser();
	
	virtual void reset();
	
	virtual IJSONParser::State parse(char c, char lookahead);
	
	//! WARNING #1: Don't forget to reset before parsing new stuff.
	//! WARNING #2: This only works if the whole content is present inside the string (because a lookahead of '\0' will be used when parsing the last character).
	virtual IJSONParser::State parse(const std::string& s);
	
	//! WARNING #1: Don't forget to reset before parsing new stuff.
	//! WARNING #2: This only works if the whole content is present inside the string (because a lookahead of '\0' will be used when parsing the last character).
	virtual IJSONParser::State parse(const char* cstr);
	
	virtual IJSONParser::State parse(const char* cstr, uint32_t length);
	
	virtual IRPCValue* stealResult();
	
	virtual IRPCValue* getResult();
	
};

#endif
