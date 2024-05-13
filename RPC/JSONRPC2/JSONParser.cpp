#include "JSONParser.h"
#include <StringHelpers.h>
#include <utf8.h>

#include <cstring>
#include <sstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <cmath>

using namespace UTF8Conversion;

std::string escapeAndQuoteJSONString(const std::string& s, bool escapeNonPrintableChars){
	std::stringstream ss;
	ss << "\"";
	for(uint32_t i=0; i<s.size(); i++){
		char c = s[i];
		if(c=='\"'){
			ss << "\\\"";
		}else if(c=='\\'){
			ss << "\\\\";
		}else if(escapeNonPrintableChars && c>=0 && c<=31){
			ss << "\\u00" << std::setfill('0') << std::setw(2) << std::hex << ((int)c) << std::dec;
		}else{
			ss << c;
		}
	}
	ss << "\"";
	return ss.str();
}

std::string convertRPCValueToJSONString(const IRPCValue& value, bool escapeNonPrintableChars, uint8_t floatingPointPrecision){
	IRPCValue::Type type = value.getType();
	if(type==IRPCValue::BOOLEAN){
		return ((const BooleanValue&)value).value?"true":"false";
	}else if(type==IRPCValue::FLOAT){
		double fvalue = ((const FloatValue&)value).value;
		if(std::isnan(fvalue) || std::isinf(fvalue)){
			std::stringstream ss; ss << "\"" << fvalue << "\"";
			return ss.str();
		}
		std::string res;
		if(floatingPointPrecision==0){
			res = convertToString<double>(fvalue);
		}else{
			std::stringstream ss; ss << std::setprecision(floatingPointPrecision) << fvalue;
			res = ss.str();
		}
		return res.find('.')==std::string::npos?(res+".0"):res;//must have a . to really be a double
	}else if(type==IRPCValue::INTEGER){
		return convertToString<int64_t>(((const IntegerValue&)value).value);
	}else if(type==IRPCValue::STRING){
		return escapeAndQuoteJSONString(((const StringValue&)value).value, escapeNonPrintableChars);
	}else if(type==IRPCValue::ARRAY){
		const ArrayValue& array = (const ArrayValue&)value;
		std::stringstream ss; ss << "[";
		for(uint32_t i=0; i<array.values.size(); i++){
			if(i>0){ss << ",";}
			ss << convertRPCValueToJSONString(*(array.values[i]), escapeNonPrintableChars, floatingPointPrecision);
		}
		ss << "]";
		return ss.str();
	}else if(type==IRPCValue::OBJECT){
		const ObjectValue& object = (const ObjectValue&)value;
		std::stringstream ss; ss << "{";
		auto begin = object.values.begin();
		for(auto it = begin; it != object.values.end(); ++it){
			if(it!=begin){ss << ",";}
			ss << "\"" << it->first << "\":" << convertRPCValueToJSONString(*(it->second), escapeNonPrintableChars, floatingPointPrecision);
		}
		ss << "}";
		return ss.str();
	}else{
		return "null";
	}
}

static inline bool isWhitespace(char c){
	return c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\0';
}

class JSONParserWithResult : public IJSONParser{
	
	protected:
	
	IRPCValue* result;
	
	void replaceResult(IRPCValue* r){
		delete result;
		result = r;
	}
	
	public:
	
	JSONParserWithResult(){
		result = NULL;
	}
	
	virtual ~JSONParserWithResult(){
		delete result;
	}
	
	virtual IRPCValue* stealResult(){
		IRPCValue* res = result;
		result = NULL;
		return res;
	}
	
	virtual IRPCValue* getResult(){
		return result;
	}
	
	virtual void deleteResult(){
		delete result;
		result = NULL;
	}
	
	virtual void reset(){
		deleteResult();
	}
	
};

class JSONStringParser : public JSONParserWithResult{
	
	private:
	
	int state;//0: beginning, 1: " read, 2: \ read, 3: closing " read, 4: error, 5-8: 4 hex digits
	std::stringstream ss;
	
	static const int maxUTF8CharLen = 7;//6 bytes + '\0' byte
	char utf8Char[maxUTF8CharLen];
	char hexDigits[4];
	
	public:
	
	JSONStringParser(){
		state = 0;
		memset(&utf8Char,0,maxUTF8CharLen);
	}
	
	void reset(){
		JSONParserWithResult::reset();
		state = 0;
		ss.str("");
	}
	
	IJSONParser::State parse(char c, char lookahead){
		switch(state){
			case 0:{
				if(c=='\"'){
					state = 1;
				}else if(!isWhitespace(c)){
					deleteResult();
					state = 4;
				}
				break;
			}case 1:{
				if(c=='\\'){
					state=2;
				}else if(c=='\"'){
					state=3;
					replaceResult(new StringValue(ss.str()));
				}else{
					ss << c;
				}
				break;
			}case 2:{
				if(c=='u'){
					state = 5;
				}else{
					ss << (c=='b'?'\b':(c=='f'?'\f':(c=='n'?'\n':(c=='r'?'\r':(c=='t'?'\t':c)))));
					state = 1;
				}
				break;
			}case 3:{
				if(!isWhitespace(c)){state = 4;}
				break;
			}case 5: case 6: case 7: case 8:{
				hexDigits[state-5] = c>='0'&&c<='9'?(c-'0'):(c>='a'&&c<='f'?(c-'a'+10):(c-'A'+10));
				state++;
				if(state==9){
					state = 1;
					uint32_t value = (((uint32_t)hexDigits[0]) << 12) | ((uint32_t)hexDigits[1]) << 8 | ((uint32_t)hexDigits[2]) << 4 | ((uint32_t)hexDigits[3]);
					uint64_t len = maxUTF8CharLen;
					char* utfCharPtr = utf8Char;
					memset(&utf8Char,0,maxUTF8CharLen);
					utf8fromcodepoint(value, &utfCharPtr, &len);
					ss << std::string(utf8Char);
				}
				break;
			}
		}
		//std::cout << "state: " << state << std::endl;
		return state==3?(IJSONParser::SUCCESS):(state==4?(IJSONParser::ERROR):(IJSONParser::PARSING));
	}
	
};

class JSONSpecialTokenParser : public JSONParserWithResult{
	
	private:
	
	const std::string token;
	int state;//error: -1, otherwise position in token
	
	std::function<IRPCValue*()> createResult;
	
	public:
	
	JSONSpecialTokenParser(const std::string& token, const std::function<IRPCValue*()>& createResult):token(token),createResult(createResult){
		state = 0;
		result = NULL;
	}
	
	void reset(){
		JSONParserWithResult::reset();
		state = 0;
	}
	
	IJSONParser::State parse(char c, char lookahead){
		int size = token.size();
		if(state==0){
			if(c==token[state]){
				state++;
				if(state==size && JSONParserWithResult::result==NULL){JSONParserWithResult::result = createResult();}
			}else if(!isWhitespace(c)){
				state = -1;
				deleteResult();
			}
		}else if(state>=size){
			if(!isWhitespace(c)){state=-1; deleteResult();}
		}else if(state>0 && state<size){
			if(c==token[state]){
				state++;
				if(state==size && JSONParserWithResult::result==NULL){JSONParserWithResult::result = createResult();}
			}else{
				state = -1;
				deleteResult();
			}
		}else{
			state = -1;
			deleteResult();
		}
		return state>=0?(state>=size?(IJSONParser::SUCCESS):(IJSONParser::PARSING)):(IJSONParser::ERROR);
	}
	
};

static inline bool isDigit(char c){
	return c>='0' && c<='9';
}

static inline bool isSign(char c){
	return c=='+' || c=='-';
}

static inline bool isExp(char c){
	return c=='e' || c=='E';
}

//! also accepts leading zeros
class JSONNumberParser : public JSONParserWithResult{
	
	private:
	
	int state;//0: start, 1: error, 2: minus/number read, 3: fraction read, 4 exponent read, 5: sign/number in exponent read
	
	bool isFloat;//true if fraction or exponent read
	std::stringstream ss;
	
	public:
	
	IJSONParser::State parse(char c, char lookahead){
		switch(state){
			case 0:{
				if(isSign(c) || isDigit(c)){
					ss << c;
					state = 2;
				}else if(c=='.'){
					ss << c;
					isFloat = true;
					state = 3;
				}else{
					state = 1;
					deleteResult();
				}
				break;
			}case 2:{
				if(c=='.'){
					ss << c;
					isFloat = true;
					state = 3;
				}else if(isExp(c)){
					ss << c;
					isFloat = true;
					state = 4;
				}else if(isDigit(c)){
					ss << c;
				}else{
					state = 1;
					deleteResult();
				}
				break;
			}case 3:{
				if(isExp(c)){
					ss << c;
					state = 4;
				}else if(isDigit(c)){
					ss << c;
				}else{
					state = 1;
					deleteResult();
				}
				break;
			}case 4:{
				if(isSign(c) || isDigit(c)){
					ss << c;
					state = 5;
				}else{
					state = 1;
					deleteResult();
				}
				break;
			}case 5:{
				if(isDigit(c)){
					ss << c;
				}else{
					state = 1;
					deleteResult();
				}
				break;
			}
		}
		bool finished = !(isDigit(lookahead)||isSign(lookahead)||isExp(lookahead)||lookahead=='.') && state!=0 && state!=1;
		if(finished){
			replaceResult(isFloat?((IRPCValue*)new FloatValue(convertStringTo<double>(ss.str()))):((IRPCValue*)new IntegerValue(convertStringTo<int64_t>(ss.str()))));
			//std::cout << "number finished: " << convertRPCValueToJSONString(*JSONParserWithResult::result) << std::endl;
		}
		return state==1?(IJSONParser::ERROR):(finished?(IJSONParser::SUCCESS):(IJSONParser::PARSING));
	}

	JSONNumberParser(){
		reset();
	}
	
	void reset(){
		JSONParserWithResult::reset();
		state = 0;
		isFloat = false;
		ss.str("");
	}

};

class JSONArrayParser : public JSONParserWithResult{

	private:
	
	int state;//0: start, 1: reading value, 2: value parsing successful/reading for delimeter, 3: ] read/success, 4: error, 5 [ read
	JSONParser* valueParser;
	
	std::list<IRPCValue*> results;
	
	public:
	
	IJSONParser::State parse(char c, char lookahead){
		switch(state){
			case 0:{
				if(c=='['){
					if(valueParser==NULL){valueParser = new JSONParser();}
					state = 5;
				}else if(!isWhitespace(c)){
					state = 4;
					deleteResult();
				}
				break;
			}case 5:{
				if(c==']'){
					replaceResult(new ArrayValue());
					state = 3;
				}else if(!isWhitespace(c)){
					IJSONParser::State s = valueParser->parse(c, lookahead);
					if(s==IJSONParser::SUCCESS){
						state = 2;
						results.push_back(valueParser->stealResult());
					}else if(s==IJSONParser::ERROR){
						state = 4;
						deleteResult();
					}else{
						state = 1;
					}
				}
				break;
			}case 1:{
				IJSONParser::State s = valueParser->parse(c, lookahead);
				if(s==IJSONParser::SUCCESS){
					state = 2;
					results.push_back(valueParser->stealResult());
				}else if(s==IJSONParser::ERROR){
					state = 4;
					deleteResult();
				}
				break;
			}case 2:{
				if(c==','){
					valueParser->reset();
					state = 5;
				}else if(c==']'){
					state = 3;
					ArrayValue* array = new ArrayValue();
					array->values = std::vector<IRPCValue*>(results.begin(), results.end());
					replaceResult(array);
					results.clear();
					//std::cout << "finished: " << convertRPCValueToJSONString(*array) << std::endl;
				}else if(!isWhitespace(c)){
					state = 4;
					deleteResult();
				}
				break;
			}case 3:{
				if(!isWhitespace(c)){
					state = 4;
					deleteResult();
				}
				break;
			}
		}
		//std::cout << "c: " << c << " state: " << state << std::endl;
		return state==3?(IJSONParser::SUCCESS):(state==4?(IJSONParser::ERROR):(IJSONParser::PARSING));
	}
	
	JSONArrayParser(){
		valueParser = NULL;
		reset();
	}
	
	~JSONArrayParser(){
		reset();
		delete valueParser;
	}
	
	void reset(){
		JSONParserWithResult::reset();
		state = 0;
		if(valueParser){valueParser->reset();}
		for(IRPCValue* r:results){delete r;}
		results.clear();
	}
	
};

class JSONObjectParser : public JSONParserWithResult{

	private:
	
	int state;//0: start, 1: { read, 2: } read/success, 3: error, 4: reading string (key), 5: expecting :, 6: reading value. 7: expecting } or , delimeters, 8: , read
	JSONParser* valueParser;
	JSONStringParser keyParser;
	
	public:
	
	IJSONParser::State parse(char c, char lookahead){
		switch(state){
			case 0:{
				if(c=='{'){
					replaceResult(new ObjectValue());
					state = 1;
				}else if(!isWhitespace(c)){
					deleteResult();
					state = 3;
				}
				break;
			}case 1:{
				if(c=='}'){
					state = 2;
				}else if(c=='\"'){
					keyParser.reset();
					auto s = keyParser.parse(c, lookahead);
					assert(s==IJSONParser::PARSING);
					state = 4;
				}else if(!isWhitespace(c)){
					deleteResult();
					state = 3;
				}
				break;
			}case 4:{
				auto s = keyParser.parse(c, lookahead);
				if(s==IJSONParser::SUCCESS){
					state = 5;
					assert(keyParser.getResult()!=NULL);
				}else if(s==IJSONParser::ERROR){
					deleteResult();
					state = 3;
				}
				break;
			}case 5:{
				if(c==':'){
					state = 6;
					//delete valueParser; valueParser = new JSONParser();
					if(valueParser){
						valueParser->reset();
					}else{
						valueParser = new JSONParser();
					}
				}else if(!isWhitespace(c)){
					deleteResult();
					state = 3;
				}
				break;
			}case 6:{
				auto s = valueParser->parse(c, lookahead);
				if(s==IJSONParser::SUCCESS){
					state = 7;
					std::unordered_map<std::string, IRPCValue*>& values = static_cast<ObjectValue*>(JSONParserWithResult::result)->values;
					std::string& key = static_cast<StringValue*>(keyParser.getResult())->value;
					if(values.find(key)==values.end()){
						values[key] = valueParser->stealResult();
					}else{//duplicate key
						deleteResult();
						state = 3;	
					}
				}else if(s==IJSONParser::ERROR){
					deleteResult();
					state = 3;
				}
				break;
			}
			case 7:{
				if(c=='}'){
					state = 2;	
				}else if(c==','){
					state = 8;
				}else if(!isWhitespace(c)){
					deleteResult();
					state = 3;
				}
				break;
			}case 8:{
				if(c=='\"'){
					keyParser.reset();
					auto s = keyParser.parse(c, lookahead);
					assert(s==IJSONParser::PARSING);
					state = 4;
				}else if(!isWhitespace(c)){
					deleteResult();
					state = 3;
				}
				break;
			}
		}
		return state==2?(IJSONParser::SUCCESS):(state==3?(IJSONParser::ERROR):(IJSONParser::PARSING));
	}
	
	JSONObjectParser(){
		valueParser = NULL;
		reset();
	}
	
	~JSONObjectParser(){
		reset();
	}
	
	void reset(){
		JSONParserWithResult::reset();
		state = 0;
		keyParser.reset();
		delete valueParser;
		valueParser = NULL;
	}
	
};

JSONParser::JSONParser(){
	state = -1;
	subParser = {
		new JSONStringParser(),
		new JSONSpecialTokenParser("true", [](){return new BooleanValue(true);}),
		new JSONSpecialTokenParser("false", [](){return new BooleanValue(false);}),
		new JSONSpecialTokenParser("null", [](){return new NULLValue();}),
		new JSONNumberParser(),
		new JSONArrayParser(),
		new JSONObjectParser()
	};
}
	
JSONParser::~JSONParser(){
	for(IJSONParser* p:subParser){
		delete p;
	}
}
	
void JSONParser::reset(){
	state = -1;
	for(IJSONParser* p:subParser){
		p->reset();
	}
}

IJSONParser::State JSONParser::parse(char c, char lookahead){
	//state: -3 success -2 error, -1 start, else parsing subParser state
	if(state==-1){
		if(isWhitespace(c)){
			return IJSONParser::PARSING;//remain in state
		}else if(c=='.' || c=='-' || (c>='0' && c<='9')){
			state = 4;
		}else{
			switch(c){
				case '{': state=6; break;
				case '[': state=5; break;
				case 'n': state=3; break;
				case 'f': state=2; break;
				case 't': state=1; break;
				case '\"': state=0; break;
				default: state=-1; return IJSONParser::ERROR;
			}
		}
		IJSONParser::State res = subParser[state]->parse(c, lookahead);
		if(res==IJSONParser::SUCCESS){state = -3;}
		return res;
	}else if(state==-3){
		if(!isWhitespace(c)){
			state = -2;
			return IJSONParser::ERROR;
		}
		return IJSONParser::SUCCESS;
	}else if(state>=0){
		IJSONParser::State res = subParser[state]->parse(c, lookahead);
		if(res==IJSONParser::SUCCESS){state = -3;}
		return res;
	}
	return IJSONParser::ERROR;
}

IJSONParser::State JSONParser::parse(const std::string& s){
	return parse(s.c_str());
}

IJSONParser::State JSONParser::parse(const char* cstr){
	//double time = getSecs();
	IJSONParser::State res = IJSONParser::PARSING;
	for(uint32_t i=0; cstr[i]!=0; i++){
		res = parse(cstr[i], cstr[i+1]);
	}
	//std::cout << "needed: " << (getSecs()-time) << std::endl;
	return res;
}

IRPCValue* JSONParser::stealResult(){
	for(IJSONParser* p:subParser){
		IRPCValue* rpcv = p->stealResult();
		if(rpcv){return rpcv;}
	}
	return NULL;
}

IRPCValue* JSONParser::getResult(){
	for(IJSONParser* p:subParser){
		IRPCValue* rpcv = p->getResult();
		if(rpcv){return rpcv;}
	}
	return NULL;
}
