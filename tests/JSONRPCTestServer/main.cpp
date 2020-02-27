#include <timing.h>
#include <StringHelpers.h>
#include <JSONRPC2Server.h>
#include <JSONRPC2Client.h>
#include <SimpleSockets.h>
#include <CRC32.h>

#include <iostream>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <list>
#include <utility>

#define RPC_PORT 62746

#define SPAM_PERIOD 1.0

#define DISCOVERY_PORT 48430

//TODO: XMetaProtocolHandler refactoren?
//TODO: UDPDiscovery refactoren?

std::pair<char*, uint32_t> stringToDiscoveryPacket(const std::string& s){
	uint32_t packetSize = s.size()+sizeof(uint32_t);
	char* packet = new char[packetSize];
	memcpy(packet, s.c_str(), s.size());
	uint32_t crc32 = htonl(crc32buf(packet, s.size()));
	memcpy(&packet[s.size()], &crc32, sizeof(uint32_t));
	return std::make_pair(packet, packetSize);	
}

std::pair<char*, uint32_t> dummyDiscoveryPacket = stringToDiscoveryPacket("REPLY Name C++-Server Serial 0 Proto RPC2_API1 Port 62746 AcceptConnection true\n");

class AlwaysSuccessXMetaHandler : public IMetaProtocolHandler{

	public:
	
	bool tryNegotiate(ISocket* socket){
		static std::string succ("000 \n");
		socket->send(succ.c_str(), succ.size());
		return true;
	}

};

class TestProcedureHandler : public IRemoteProcedureCaller, IRemoteProcedureCallReceiver{

	private:
	
	bool shallSpam;
	double lastSpamTime;

	public:
	
	JSONRPC2Client* client;
	
	enum ProcedureCalls{
		SUM,
		CALL_COUNT
	};

	TestProcedureHandler(JSONRPC2Client* client):shallSpam(false),lastSpamTime(0.0),client(client){
		client->registerCallReceiver("moveXY", this);
		client->registerCallReceiver("registerForSpam", this);
	}
	
	~TestProcedureHandler(){
		delete client;
	}

	void OnProcedureResult(IRPCValue* results, uint32_t id){
		if(id==SUM){
			std::cout << "sum result received: " << convertRPCValueToJSONString(*results) << std::endl;
		}
		delete results;
	}
	
	void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		delete errorData;
	}
	
	int moveXY(int a, int b){
		return a+b;
	}

	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
		if(procedure.compare("moveXY")==0){
			return createRPCValue(moveXY(createNativeValue<int>(values[0]), createNativeValue<int>(values[1])));
		}else if(procedure.compare("registerForSpam")==0){
			shallSpam = true;
		}
		return NULL;
	}
	
	void update(){
		client->update();
		if(shallSpam){
			double t = getSecs();
			if(t-lastSpamTime>1.0){
				lastSpamTime = t;
				client->callRemoteProcedure("sum", std::vector<IRPCValue*>{createRPCValue(1), createRPCValue(2)}, this, SUM);
			}
		}
	}

};

int main(int argc, char *argv[]){
	
	AlwaysSuccessXMetaHandler handler;
	JSONRPC2Server server(RPC_PORT, 2000, &handler);
	
	std::list<TestProcedureHandler> handlers;
	
	//Dummy Discover stuff
	IPv6UDPSocket discoverySocket;
	bool success = discoverySocket.bind(DISCOVERY_PORT);
	if(!success){
		std::cout << "Error: " << strerror(errno) << std::endl;
	}
	uint32_t bufSize = 512;
	char buf[bufSize];
	
	while(true){
		//Connection management of JSONRPC2 stuff
		JSONRPC2Client* newClient = server.accept();
		if(newClient){
			handlers.emplace_back(newClient);
		}
		auto it = handlers.begin();
		while(it!=handlers.end()){
			TestProcedureHandler& handler = *it;
			handler.update();
			if(!handler.client->isConnected()){
				std::cout << "Client disconnected" << std::endl;
				it = handlers.erase(it);
				continue;
			}
			++it;
		}
		//Dummy Discovery stuff
		uint32_t received = discoverySocket.recv(buf, bufSize);
		if(received>0){
			IPv6Address fromAddress = discoverySocket.getLastDatagramAddress();
			std::cout << "received: " << std::string(buf, received) << " from " << fromAddress.getAddressAsString() << ":" << fromAddress.getPort() << std::endl << std::flush;
			discoverySocket.setUDPTarget(fromAddress);
			discoverySocket.send(dummyDiscoveryPacket.first, dummyDiscoveryPacket.second);
		}
		delay(10);
	}
	
	return 0;
}
