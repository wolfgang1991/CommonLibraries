#include <JSONRPC2Client.h>
#include <CurlRPCSocket.h>
#include <timing.h>

#include <iostream>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <list>
#include <utility>

class TestCaller : public IRemoteProcedureCaller{

	private:
	
	IRPCClient* jc;

	public:
	
	TestCaller(IRPCClient* jc):jc(jc){}
	
	void test(){
		jc->callRemoteProcedure("moveXY", std::vector<IRPCValue*>{new IntegerValue(10), new IntegerValue(20)}, this, 123);
		jc->callRemoteProcedure("notExistingMethod", std::vector<IRPCValue*>{new BooleanValue(true), new IntegerValue(666), new FloatValue(2.9979e8), new StringValue("lalala"), new ArrayValue{new IntegerValue(1), new IntegerValue(2)}, new ObjectValue{ {"one", new IntegerValue{1}}, {"twoPointThree", new FloatValue{2.3}}}}, this, 456);
	}
	
	void OnProcedureResult(IRPCValue* results, uint32_t id){
		std::cout << "id: " << id << " received: " << convertRPCValueToJSONString(*results) << std::endl << std::flush;
		delete results;
	}

};

int main(int argc, char *argv[]){

	JSONRPC2Client jc;

	CurlRPCSocket socket("https://localhost/rpcfwd", false, false);
	
	jc.useSocket(&socket, 10000, 1000);
	
	TestCaller caller(&jc);
	
	caller.test();
	
	while(true){
		jc.update();
		delay(1);
	}

	return 0;
}
