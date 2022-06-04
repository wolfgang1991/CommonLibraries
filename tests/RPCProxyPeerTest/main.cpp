#include <JSONRPC2Client.h>
#include <RPCProxyPeer.h>
#include <StringHelpers.h>
#include <timing.h>

#include <cassert>
#include <iostream>

#define RPC_PORT 62746

class AlwaysSuccessXMetaHandler : public IMetaProtocolHandler{

	public:
	
	bool tryNegotiate(ICommunicationEndpoint* socket){
		return true;
	}
	
	bool useCompression() const{return true;}

};

enum TestAPIFunctions{
	CALC_SUM,
	TEST_API_FUNCTIONS_COUNT
};

class TestCaller : public IRemoteProcedureCaller{
	
	public:
	
	 void OnProcedureResult(IRPCValue* results, uint32_t id){
	 	if(id==CALC_SUM){
	 		std::cout << "sum result received: " << createNativeValue<int32_t>(results) << std::endl;
	 	}
	 	delete results;
	 }
	
};

class TestCallReceiver : public IRemoteProcedureCallReceiver{
	
	public:
	
	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
		std::cout << "procedure: " << procedure << std::endl;
		if(procedure=="calcSum"){
			return createRPCValue(createNativeValue<int32_t>(values[0])+createNativeValue<int32_t>(values[1]));
		}
		return NULL;
	}
	
};

void calcSum(IRPC* rpc, int32_t a, int32_t b, IRemoteProcedureCaller* caller, uint32_t id = CALC_SUM){
	rpc->callRemoteProcedure("calcSum", std::vector<IRPCValue*>{createRPCValue(a), createRPCValue(b)}, caller, id);
}

int main(int argc, char *argv[]){

	//Create underlying RPC connection
	AlwaysSuccessXMetaHandler handler;

	JSONRPC2Client jc;
	jc.connect(IPv6Address("::1", RPC_PORT), 1000, 2000, 1000, &handler);
	
	std::cout << "Waiting for connection..." << std::endl;
	while(jc.getState()==IRPCClient::CONNECTING){
		jc.update();
		delay(1);
	}
	
	assert(jc.getState()==IRPCClient::CONNECTED);
	
	//Create proxy peer
	RPCProxyPeer peer(&jc);
	std::string service = convertToString(getEpochSecs());
	peer.registerPeerService(service);
	std::cout << "registered service: " << service << std::endl;
	
	std::unordered_set<std::string> connectedRemoteServices;
	std::list<RPCPeerConnection*> connectionsToRemote;
	
	std::list<RPCPeerConnection*> connectionsFromRemote;
	
	TestCallReceiver receiver;
	TestCaller caller;
	int32_t a = 0, b = 10;
	
	double lastRequestTime = 0.0, lastUseServiceTime = 0.0;
	while(true){
		//Find services and connect to new ones (also connects to itself)
		double t = getSecs();
		if(t-lastRequestTime>5.0){
			lastRequestTime = t;
			peer.requestServiceNames([&connectedRemoteServices, &connectionsToRemote, &peer, &service](const std::vector<std::string>& v){
				std::cout << "Found services: " << std::endl;
				for(const std::string& s : v){
					std::cout << s << std::endl;
					if(s!=service){//don't connect to own service => problems
						auto it = connectedRemoteServices.find(s);
						if(it==connectedRemoteServices.end()){
							connectedRemoteServices.insert(s);
							connectionsToRemote.push_back(peer.connectToPeer(s));
						}
					}
				}
			});
		}
		//Update connections to remote service
		bool useServices = t-lastUseServiceTime>2.0;
		if(useServices){lastUseServiceTime = t;}
		auto it = connectionsToRemote.begin();
		while(it != connectionsToRemote.end()){
			RPCPeerConnection* c = *it;
			if(!c->isConnected()){
				std::cout << "Connection to remote lost: " << c->getServiceName() << std::endl;
				connectedRemoteServices.erase(c->getServiceName());
				it = connectionsToRemote.erase(it);
				delete c;
			}else{
				if(useServices){
					std::cout << "call calcSum(" << a << "," << b << ") on service: " << c->getServiceName() << std::endl;
					calcSum(c, a, b, &caller);
					a++;
				}
				++it;
			}
		}
		//Handle connection to own service
		RPCPeerConnection* newConnection = peer.accept();
		if(newConnection){
			connectionsFromRemote.push_back(newConnection);
			newConnection->registerCallReceiver("calcSum", &receiver);
		}
		it = connectionsFromRemote.begin();
		while(it != connectionsFromRemote.end()){
			RPCPeerConnection* c = *it;
			if(!c->isConnected()){
				std::cout << "Connection from remote lost: " << c->getServiceName() << std::endl;
				it = connectionsFromRemote.erase(it);
			}else{
				++it;
			}
		}
		//update peer
		peer.update();
	}
	

	return 0;
}
