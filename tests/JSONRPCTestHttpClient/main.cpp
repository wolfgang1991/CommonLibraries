#include <RequestBasedJSONRPC2Client.h>
#include <timing.h>
#include <JSONParser.h>

#include <cassert>
#include <csignal>
#include <iostream>

volatile bool running = true;

void sigfunc(int sig){
	running = false;
}

class TestCaller : public IRemoteProcedureCaller{

	private:
	
	IRPC* jc;

	public:
	
	TestCaller(IRPC* jc):jc(jc){}
	
	void test(){
		jc->callRemoteProcedure("moveXY", std::vector<IRPCValue*>{
			new IntegerValue(10), 
			new IntegerValue(20)
		}, this, 123);
		jc->callRemoteProcedure("notExistingMethod", std::vector<IRPCValue*>{
			new BooleanValue(true),
			new IntegerValue(666),
			new FloatValue(2.9979e8),
			new StringValue("lalala"),
			new ArrayValue{
				new IntegerValue(1),
				new IntegerValue(2)
			},
			new ObjectValue{
				{"one", new IntegerValue{1}},
				{"twoPointThree", new FloatValue{2.3}}
			}
		}, this, 456);
	}
	
	void OnProcedureResult(IRPCValue* results, uint32_t id){
		std::cout << "id: " << id << " received: " << convertRPCValueToJSONString(*results) << std::endl;
		delete results;
	}
	
	void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		std::cout << "errorCode: " << errorCode << " errorMessage: " << errorMessage << " id: " << id << " errorData: " << (errorData==NULL?"null":convertRPCValueToJSONString(*errorData)) << std::endl;
		delete errorData;
	}

};


int main(int argc, char *argv[]){

	signal(SIGINT, sigfunc);

	//CURLRequestSender sender("http://localhost/rpc", true, true);
	CURLRequestSender sender("http://127.0.0.1:34634/rpc", true, true);
	RequestBasedJSONRPC2Client client(&sender, 0);
	
	TestCaller tc(&client);
	
	tc.test();
	
	while(running){
		client.update();
		delay(1000);
		tc.test();
	}
	
	std::cout << "exit" << std::endl;
	
	return 0;
}
