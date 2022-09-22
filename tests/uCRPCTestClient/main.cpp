#include "../uCRPCTestServer/testapi.h"

#include <uCArray.h>
#include <SimpleSockets.h>
#include <timing.h>

#include <iostream>
#include <cassert>

using namespace UCRPC;

class TestCaller : public IUCRemoteProcedureCaller{
		
	public:
	
	void OnProcedureResult(uint8_t* result, uint16_t resultLength, uint16_t functionID){
		if(functionID==ADD_TESTVALUES || functionID==SUBTRACT_TESTVALUES2){
			int32_t value;
			if(deserializeResult(result, resultLength, value)){
				std::cerr << "received result: " << value << std::endl;
			}else{
				std::cerr << "deserializeResult error: " << functionID << std::endl;
			}
		}else if(functionID==CONCAT_STRINGS){
			ApiString value; deserializeResult(result, resultLength, value);
			std::cout << "concat strings result: " << value.convertToStdString() << std::endl;
		}
	}
	
	void OnProcedureError(ProcedureError error, uint16_t functionID){
		std::cerr << "OnProcedureError: functionID: " << functionID << " error: " << (int)error << std::endl;
	}
	
};

int main(int argc, char *argv[]){

	IPv4UDPSocket s;
	s.bind(7563);
	s.setUDPTarget(IPv4Address("127.0.0.1", 7564));
	
	ucstd::array<RegFunc, 2> functions{};
	
	TestCaller caller;
	
	::UCRPC::UCRPC<uint16_t, IPv4UDPSocket, maxSpaceReq, 200, decltype(functions)> ucrpc(s, functions);
	
	double lastTime = getSecs();
	
	while(true){
		double t = getSecs();
		if(t-lastTime>1.0){
			addTestValues(&ucrpc, TestValues{1,2,3,4,5,6}, &caller);
			subtractTestValues2(&ucrpc, TestValues2{1000,100,10}, &caller);
			concatStrings(&ucrpc, ApiString("bedolf\xaa"), ApiString("\xcc-cedolf"), &caller);
			lastTime = t;
		}
		ucrpc.update();
		delay(1);
	}
	
	return 0;
}
