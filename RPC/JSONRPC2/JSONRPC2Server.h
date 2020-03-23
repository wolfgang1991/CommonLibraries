#ifndef JSONRPC2Server_H_INCLUDED
#define JSONRPC2Server_H_INCLUDED

#include <IRPC.h>

class JSONRPC2Client;
class IPv6TCPSocket;

//! Listens for connections, negotiates the protocol if applicable and returns the server-side JSONRPC2Client representation for a newly connected client
class JSONRPC2Server{

	private:
	
	IPv6TCPSocket* serverSocket;
	bool good;
	IMetaProtocolHandler* handler;
	uint32_t pingTimeout;
	
	public:
	
	//! creates the server socket, binds and starts listening
	JSONRPC2Server(uint16_t port, uint32_t pingTimeout, IMetaProtocolHandler* handler = NULL, int maxPendingConnections = 10);
	
	//! note: handler won't be deleted
	~JSONRPC2Server();
	
	//! true if socket create, bound and listening successful
	bool isGood();
	
	//! timeout in ms (if NOT equal 0 it blocks until there's something to accept or timeout)
	//! peerAddress: if not NULL it will be filled with the peer address
	JSONRPC2Client* accept(uint32_t timeout = 0, IPv6Address* peerAddress = NULL);
	
};

#endif
