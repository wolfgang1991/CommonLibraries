#include <JSONRPC2Client.h>
#include <timing.h>

#include <iostream>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <list>
#include <utility>

#define RPC_PORT 62746

class TestCaller : public IRemoteProcedureCaller{

	private:
	
	IRPCClient* jc;

	public:
	
	TestCaller(IRPCClient* jc):jc(jc){}
	
	void test(){
		jc->callRemoteProcedure("moveXY", std::vector<IRPCValue*>{new IntegerValue(10), new IntegerValue(20)}, this, 123);
		jc->callRemoteProcedure("registerForSpam", std::vector<IRPCValue*>{}, this, 124);
		jc->callRemoteProcedure("notExistingMethod", std::vector<IRPCValue*>{new BooleanValue(true), new IntegerValue(666), new FloatValue(2.9979e8), new StringValue("lalala"), new ArrayValue{new IntegerValue(1), new IntegerValue(2)}, new ObjectValue{ {"one", new IntegerValue{1}}, {"twoPointThree", new FloatValue{2.3}}}}, this, 456);
	}
	
	void OnProcedureResult(IRPCValue* results, uint32_t id){
		std::cout << "id: " << id << " received: " << convertRPCValueToJSONString(*results) << std::endl << std::flush;
		delete results;
	}

};

class TestReceiver : public IRemoteProcedureCallReceiver{

	IRPCClient* jc;

	public:
	
	TestReceiver(IRPCClient* jc){
		this->jc = jc;
		jc->registerCallReceiver("sum", this);
		jc->registerCallReceiver("test", this);
	}
	
	~TestReceiver(){
		jc->unregisterCallReceiver("sum");
	}
	
	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
		if(procedure.compare("sum")==0){
			int64_t sum = 0;
			for(uint32_t i=0; i<values.size(); i++){
				IRPCValue* v = values[i];
				assert(v->getType()==IRPCValue::INTEGER);
				sum += ((IntegerValue*)v)->value;
			}
			return new IntegerValue(sum);
		}else if(procedure.compare("test")==0){
			std::cout << "FROM TEST PROCEDURE:\n";
			for(uint32_t i=0; i<values.size(); i++){
				std::cout << "value[" << i << "]: " << convertRPCValueToJSONString(*values[i]) << std::endl;
			}
			std::cout << "END OF TEST PROC. OUTPUT\n" << std::flush;
		}
		return NULL;
	}

};

class AlwaysSuccessXMetaHandler : public IMetaProtocolHandler{

	public:
	
	bool tryNegotiate(ISocket* socket){
		return true;
	}
	
	bool useCompression() const{return true;}

};

int main(int argc, char *argv[]){

	AlwaysSuccessXMetaHandler handler;

	JSONRPC2Client jc;
	jc.connect(IPv6Address("::1", RPC_PORT), 1000, 2000, 1000, &handler);
	
	std::cout << "Waiting for connection..." << std::endl;
	while(jc.getState()==IRPCClient::CONNECTING){
		jc.update();
		delay(1);
	}
	
	TestCaller caller(&jc);
	TestReceiver receiver(&jc);
	
	
	if(jc.getState()==IRPCClient::CONNECTED){
		caller.test();
		while(true){
			jc.update();
			if(jc.getState()==IRPCClient::NOT_CONNECTED){
				std::cout << "Connection lost, exiting ..." << std::endl;
				return 0;
			}
			delay(1);
		}
	}else{
		std::cout << "Connection error" << std::endl;
	}

	return 0;
}
