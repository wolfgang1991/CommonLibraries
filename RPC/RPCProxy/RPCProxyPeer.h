#ifndef RPCProxyPeer_H_INCLUDED
#define RPCProxyPeer_H_INCLUDED

#include <IRPC.h>

#include <functional>

class RPCProxyPeerPrivate;
class RPCPeerConnection;
class RPCPeerConnectionPrivate;

//! A peer can offer services and connect to other peers services. Each service peer connection is handled separately via RPCPeerConnection.
//! A peer registers it's service at the proxy server and other peers can connect to this service via the proxy server.
//! Note it is not possible to connect a peer to it's own services.
class RPCProxyPeer{
	friend class RPCPeerConnection;

	private:
	
	RPCProxyPeerPrivate* p;
	
	public:
	
	//! proxyRPC: underlying rpc connection to proxy, not owned by RPCProxyPeer
	RPCProxyPeer(IMinimalRPCClient* proxyRPC);
	
	virtual ~RPCProxyPeer();
	
	//! registers an own service for the given name (must be unique)
	void registerPeerService(const std::string& name);
	
	//! returns NULL if no new peer connected
	RPCPeerConnection* accept();
	
	RPCPeerConnection* connectToPeer(const std::string& serviceName);
	
	void requestServiceNames(const std::function<void(const std::vector<std::string>&)>& onReceivedCallback = [](const std::vector<std::string>&){});
	
	//! the last received from the RPCProxyServer
	const std::vector<std::string>& getServiceNames();
	
	//! updates the underlying proxyRPC
	void update();
	
};

//! Represents a rpc connection between two peers via a service.
class RPCPeerConnection : public IMinimalRPCClient{
	friend class RPCProxyPeer;
	friend class RPCProxyPeerPrivate;
	
	private:
	
	RPCPeerConnectionPrivate* p;
	
	RPCProxyPeer* peer;
	std::string serviceName;
	int32_t id;
	
	//! create for existing connection / connected by remote
	RPCPeerConnection(RPCProxyPeer* peer, const std::string& serviceName, int32_t id);
	
	//! create for pending connection / connection request to remote
	RPCPeerConnection(RPCProxyPeer* peer, const std::string& serviceName);
	
	//! just removes local connection information, called when disconnected remotely and inside regular disconnect
	void disconnectSilent();
	
	//! calls the appropriate registered receiver
	IRPCValue* callLocally(const std::string& procedure, const std::vector<IRPCValue*>& values);
	
	//! set the id and state when a pending conenction is now truly connected
	void OnTrulyConnected(int32_t connID);
	
	void OnConnectionError();
	
	public:
	
	virtual ~RPCPeerConnection();
	
	int32_t getConnectionID() const;
	
	const std::string& getServiceName() const;
	
	bool callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller = NULL, uint32_t id = 0, bool deleteValues = true);
	
	void registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver);
	
	void unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver);
	
	void removeProcedureCaller(IRemoteProcedureCaller* caller);
	
	//! not necessary to call / gets updated by RPCProxyPeer
	void update();

	ClientState getState() const;

	void disconnect();
		
};

#endif
