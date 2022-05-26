#ifndef RPCProxyPeer_H_INCLUDED
#define RPCProxyPeer_H_INCLUDED

#include <IRPC.h>

#include <functional>

class RPCProxyPeerPrivate;
class RPCPeerConnection;
class RPCPeerConnectionPrivate;

class RPCProxyPeer{
	friend class RPCPeerConnection;

	private:
	
	RPCProxyPeerPrivate* p;
	
	public:
	
	//! proxyRPC: underlying rpc connection to proxy
	RPCProxyPeer(IRPC* proxyRPC);
	
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

class RPCPeerConnection : public IRPCClient{
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
	
	public:
	
	virtual ~RPCPeerConnection();
	
	int32_t getConnectionID() const;
	
	const std::string& getServiceName() const;
	
	bool callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller = NULL, uint32_t id = 0, bool deleteValues = true);
	
	void registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver);
	
	void unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver);
	
	void removeProcedureCaller(IRemoteProcedureCaller* caller);
	
	void update();
	
	void connect(const IIPAddress& address, uint32_t pingSendPeriod, uint32_t pingTimeout, uint32_t connectTimeout, IMetaProtocolHandler* metaProtocolHandler = NULL);

	ClientState getState() const;

	bool isConnected() const;

	void disconnect();

	double getLastReceiveTime();

	void flush();
		
};

#endif
