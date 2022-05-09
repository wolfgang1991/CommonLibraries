#ifndef RPCProxyServer_H_INCLUDED
#define RPCProxyServer_H_INCLUDED

#include <IRPC.h>

class RPCProxyServerPrivate;

class RPCProxyServer{
	friend class IRemoteProcedureCaller;
	
	private:
	
	RPCProxyServerPrivate* p;
	
	public:
	
	RPCProxyServer();
	
	virtual ~RPCProxyServer();
	
	//! needs to be called when a new rpc connection has been established (see JSONRPC2Server as example, not used here because protocol agnostic)
	void addPeer(IRPCClient* rpc);
	
	//! removes clients when connection lost, 
	void update();
	
};

#endif
