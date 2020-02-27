#include "JSONRPC2Server.h"
#include "JSONRPC2Client.h"

#include <SimpleSockets.h>

#include <iostream>

//! Returns always NULL in the callProcedure method to provide the ping mechanism
class PingReceiver : public IRemoteProcedureCallReceiver{
	
	public:
	
	virtual IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values);
	
};

static PingReceiver pingReceiver;

JSONRPC2Server::JSONRPC2Server(uint16_t port, uint32_t pingTimeout, IMetaProtocolHandler* handler, int maxPendingConnections){
	serverSocket = new IPv6TCPSocket();
	good = serverSocket->bind(port);
	if(good){
		good = serverSocket->listen(maxPendingConnections);
	}
	this->pingTimeout = pingTimeout;
	this->handler = handler;
}
	
JSONRPC2Server::~JSONRPC2Server(){
	delete serverSocket;
}
	
bool JSONRPC2Server::isGood(){
	return good;
}
	
JSONRPC2Client* JSONRPC2Server::accept(uint32_t timeout){
	IPv6TCPSocket* clientSocket = serverSocket->accept(timeout);
	if(clientSocket){
		if(handler){
			bool res = handler->tryNegotiate(clientSocket);
			if(!res){
				std::cerr << "Negotiation unsuccessful" << std::endl;
				delete clientSocket;
				return NULL;
			}
		}
		JSONRPC2Client* client = new JSONRPC2Client();
		client->useSocket(clientSocket, pingTimeout, PING_DISABLE_SEND_PERIOD);
		client->registerCallReceiver("rc:ping", &pingReceiver);
		return client;
	}
	return NULL;
}

IRPCValue* PingReceiver::callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
	return NULL;
}
