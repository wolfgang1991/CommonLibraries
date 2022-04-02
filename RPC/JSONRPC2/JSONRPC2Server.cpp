#include "JSONRPC2Server.h"
#include "JSONRPC2Client.h"

#include <SimpleSockets.h>
#include <ZSocket.h>

#include <iostream>

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
	
JSONRPC2Client* JSONRPC2Server::accept(uint32_t timeout, IPv6Address* peerAddress){
	ICommunicationEndpoint* clientSocket = serverSocket->accept(timeout, peerAddress);
	if(clientSocket){
		if(handler){
			bool res = handler->tryNegotiate(clientSocket);
			if(!res){
				std::cerr << "Negotiation unsuccessful" << std::endl;
				delete clientSocket;
				return NULL;
			}
			if(handler->useCompression()){
				clientSocket = new ZSocket(clientSocket);
			}
		}
		JSONRPC2Client* client = new JSONRPC2Client();
		client->useSocket(clientSocket, pingTimeout, PING_DISABLE_SEND_PERIOD);
		return client;
	}
	return NULL;
}
