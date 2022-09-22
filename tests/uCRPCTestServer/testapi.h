#ifndef testapi_H_
#define testapi_H_

#include <uCRPC.h>

enum UCRPCFunctions{
	ADD_TESTVALUES,
	SUBTRACT_TESTVALUES2,
	CONCAT_STRINGS,
	FUNCTION_COUNT
};

struct TestValues{
	
	int32_t a;
	int16_t b;
	int8_t c;
	int8_t d;
	int8_t e;
	int8_t f;

	UCRPC_FUNCTIONS(a, b, c, d, e, f)
	
};

struct TestValues2{
	
	int32_t a;
	int16_t b;
	int8_t c;

	UCRPC_FUNCTIONS(a, b, c)
	
};

using ApiString = UCRPC::String<128>;

struct ConcatStringParams{
	
	ApiString a;
	ApiString b;
	
	UCRPC_FUNCTIONS(a, b)
	
};

constexpr uint16_t maxSpaceReq = GET_MAX_SPACE_REQUIREMENT(TestValues, TestValues2, ApiString, ConcatStringParams);
using RegFunc = UCRPC::RegisteredFunction<uint16_t, maxSpaceReq>;

template<typename TRPC>
void addTestValues(TRPC* rpc, const TestValues& values, UCRPC::IUCRemoteProcedureCaller* caller = nullptr){
	rpc->callRemoteProcedure(ADD_TESTVALUES, values, caller);
}

template<typename TRPC>
void subtractTestValues2(TRPC* rpc, const TestValues2& values, UCRPC::IUCRemoteProcedureCaller* caller = nullptr){
	rpc->callRemoteProcedure(SUBTRACT_TESTVALUES2, values, caller);
}

//! returns a+b
template<typename TRPC>
void concatStrings(TRPC* rpc, const ApiString& a, const ApiString& b, UCRPC::IUCRemoteProcedureCaller* caller = nullptr){
	rpc->callRemoteProcedure(CONCAT_STRINGS, ConcatStringParams{a, b}, caller);
}

#endif
