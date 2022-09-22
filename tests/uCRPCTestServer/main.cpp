#include "testapi.h"

#include <uCArray.h>
#include <SimpleSockets.h>
#include <timing.h>

#include <iostream>
#include <cassert>

class TestReceiver : public UCRPC::IUCRemoteProcedureCallReceiver<uint16_t,maxSpaceReq>{
		
	public:
	
	void callProcedure(UCRPC::IUCRPC<uint16_t,maxSpaceReq>* ucrpc, uint16_t functionID, uint8_t* parameter, uint16_t parameterLength) override{
		if(functionID==ADD_TESTVALUES){
			std::cout << "ADD_TESTVALUES" << std::endl;
			TestValues v; UCRPC_DESERIALIZE_PARAMS(v, uint16_t)
			int32_t result = v.a+v.b+v.c+v.d+v.e+v.f;
			ucrpc->returnValue(result);
		}else if(functionID==SUBTRACT_TESTVALUES2){
			std::cout << "SUBTRACT_TESTVALUES2" << std::endl;
			TestValues2 v; UCRPC_DESERIALIZE_PARAMS(v, uint16_t)
			int32_t result = v.a-v.b-v.c;
			ucrpc->returnValue(result);
		}else if(functionID==CONCAT_STRINGS){
			ConcatStringParams v; UCRPC_DESERIALIZE_PARAMS_WITH_STRINGS(v, uint16_t)
			std::cout << "CONCAT_STRINGS: " << v.a.convertToStdString() << " + " << v.b.convertToStdString() << std::endl;
			ApiString s(v.a.convertToStdString() + v.b.convertToStdString());
			ucrpc->returnValue(s);
		}else{
			ucrpc->returnError(UCRPC::IUCRemoteProcedureCaller::NOT_FOUND);
		}
	}
	
};

template<typename TArray>
void printArray(TArray& a){
	for(size_t i=0; i<a.size(); i++){
		std::cout << a[i] << ", ";
	}
	std::cout << std::endl;
}

int main(int argc, char *argv[]){

	ucstd::array<int,4> test{5,2,7,3};
	ucstd::array<int,4> sorted{2,3,5,7};
	test.sort();
	printArray(test);
	assert(test==sorted);
	test.sort();
	assert(test==sorted);
	assert(test.findBisect(2)==0);
	assert(test.findBisect(3)==1);
	assert(test.findBisect(5)==2);
	assert(test.findBisect(7)==3);
	assert(test.findBisect(100)==test.size());
	assert(test.findBisect(4)==test.size());
	assert(test.findBisect(0)==test.size());
	
	ucstd::array<int,0> test2;
	test2.sort();
	assert(test2.findBisect(100)==test2.size());
	
	ucstd::array<int,1> test3{4};
	test3.sort();
	assert(test3.findBisect(4)==0);
	assert(test3.findBisect(100)==test3.size());

	std::cout << "maxSpaceReq: " << maxSpaceReq << std::endl;

	IPv4UDPSocket s;
	s.bind(7564);
	s.setUDPTarget(IPv4Address("127.0.0.1", 7563));
	
	TestReceiver receiver;
	
	ucstd::array<RegFunc, 3> functions{
		RegFunc{SUBTRACT_TESTVALUES2, &receiver},
		RegFunc{ADD_TESTVALUES, &receiver},
		RegFunc{CONCAT_STRINGS, &receiver}
	};
	
	UCRPC::UCRPC<uint16_t, IPv4UDPSocket, maxSpaceReq, 200, decltype(functions)> ucrpc(s, functions);
	
	while(true){
		ucrpc.update();
		delay(1);
	}
	
	return 0;
}
