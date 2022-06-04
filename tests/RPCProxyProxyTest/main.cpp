#include <JSONRPC2Server.h>
#include <JSONRPC2Client.h>
#include <RPCProxyServer.h>
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


int main(int argc, char *argv[]){

	//Create underlying RPC connection
	AlwaysSuccessXMetaHandler handler;
	JSONRPC2Server rpcServer(RPC_PORT, 2000, &handler, 10);
	assert(rpcServer.isGood());
	
	//Create proxy server
	RPCProxyServer proxyServer;
	
	while(true){
		JSONRPC2Client* newClient = rpcServer.accept();
		if(newClient){
			proxyServer.addPeer(newClient);
		}
		proxyServer.update();
		delay(1);//us sth. more sophisticated on real server
	}

	return 0;
}
