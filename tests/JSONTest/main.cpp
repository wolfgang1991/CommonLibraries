#include <JSONParser.h>
#include <JSONRPC2Client.h>

#include <csignal>
#include <cassert>
#include <iostream>

static void checkResult(JSONParser& parser, const std::string& toParse, IRPCValue* expected = NULL, IJSONParser::State expectedState = IJSONParser::SUCCESS){
	IJSONParser::State s = parser.parse(toParse);
	assert(s==expectedState);
	if(s==IJSONParser::SUCCESS){
		IRPCValue* toCheck = parser.stealResult();
		assert(toCheck!=NULL);
		std::string tc = convertRPCValueToJSONString(*toCheck);
		std::cout << tc << std::endl;
		if(expected){
			if(!tc.compare(convertRPCValueToJSONString(*expected))==0){
				std::cout << "\t-> Error: Does not match the expectation: " << convertRPCValueToJSONString(*expected) << std::endl;
				raise(SIGINT);
			}
		}
		delete toCheck;
	}
	parser.reset();
	delete expected;
}

int main(int argc, char *argv[]){

	JSONParser parser;
	
	checkResult(parser, " ", NULL, IJSONParser::PARSING);
	checkResult(parser, "\"test\"", new StringValue("test"));
	checkResult(parser, "\"a\\\\\\u0230\"", new StringValue("a\\Ȱ"));
	checkResult(parser, " \n \"\\n\" \t", new StringValue("\n"));
	checkResult(parser, " \tfalse\t ", new BooleanValue(false));
	checkResult(parser, " \ttrue\t ", new BooleanValue(true));
	checkResult(parser, "null", new NULLValue());
	checkResult(parser, "1", new IntegerValue(1));
	checkResult(parser, "-10", new IntegerValue(-10));
	checkResult(parser, "1.5e2", new FloatValue(1.5e2));
	checkResult(parser, " -.3\t", new FloatValue(-.3));
	checkResult(parser, " [1, 2, 3 ,4]\t", new ArrayValue{new IntegerValue(1), new IntegerValue(2), new IntegerValue(3), new IntegerValue(4)});
	checkResult(parser, "[]", new ArrayValue());
	checkResult(parser, " [1, 2, [\"a\",true],\t[]\t]\t", new ArrayValue{new IntegerValue(1), new IntegerValue(2), new ArrayValue{new StringValue("a"), new BooleanValue(true)}, new ArrayValue()});
	checkResult(parser, "{\"a\":1,\"b\":[] , \t\n\r \"c\":true\t}", new ObjectValue{{"a", new IntegerValue(1)}, {"b", new ArrayValue()}, {"c", new BooleanValue(true)}});
	checkResult(parser, "{\"test\" :\t{\"a\":1.25 },\"b\":{}}", new ObjectValue{{"test", new ObjectValue{{"a", new FloatValue(1.25)}}}, {"b", new ObjectValue()}});
	checkResult(parser, "{\"\":\"\"}", new ObjectValue{{"", new StringValue("")}});
	checkResult(parser, "{\"a\":\"a\",\"a\":\"b\"}", NULL, IJSONParser::ERROR);
	checkResult(parser, "{\"a\":\"a\",\"b\":\"b\"}", new ObjectValue{{"a", new StringValue("a")}, {"b", new StringValue("b")}});

	return 0;
}
