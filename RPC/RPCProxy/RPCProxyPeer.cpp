#include "RPCProxyPeer.h"

#include <UniqueIdentifierGenerator.h>

#include <set>

class RPCPeerConnectionPrivate{

	public:
	
	struct Call : public IRemoteProcedureCaller{
		
		RPCPeerConnectionPrivate* p;
		int32_t callID;
		
		void OnProcedureResult(IRPCValue* results, uint32_t id){//only used for rc:call
			callID = createNativeValue<int32_t>(results);
			delete results;
			if(callID<0){//error
				//TODO call OnProcedureError
				p->calls.erase(this);
				delete this;
			}
		}
		
	};
	
	std::set<Call*> calls;
	
	~RPCPeerConnectionPrivate(){
		for(Call* c : calls){
			delete c;
		}
	}
	
};

int32_t RPCPeerConnection::getConnectionID() const{
	return id;
}
		
const std::string& RPCPeerConnection::getServiceName() const{
	return serviceName;
}

RPCPeerConnection::RPCPeerConnection(RPCProxyPeer* peer, const std::string& serviceName, int32_t id):peer(peer),serviceName(serviceName),id(id){
	p = new RPCPeerConnectionPrivate();
}

RPCPeerConnection::~RPCPeerConnection(){
	delete p;
}

bool RPCPeerConnection::callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller, uint32_t id, bool deleteValues){
	ArrayValue* a = new ArrayValue();
	a->values = values;
	RPCPeerConnectionPrivate::Call* c = new RPCPeerConnectionPrivate::Call();
	p->calls.insert(c);
	//TODO wrong: (rc:return carries the return value): peer->p->proxyRPC->callRemoteProcedure("rc:call", std::vector<IRPCValue*>{createRPCValue(id), createRPCValue(procedure), a}, c);
}

//TODO RPCPeerConnection functions

class RPCProxyPeerPrivate : public IRemoteProcedureCaller{
	
	public:
	
	IRPC* proxyRPC;
	
	std::unordered_map<std::string, LambdaCallReceiver> proxyFunctions;
	
	std::map<int32_t, RPCPeerConnection*> pendingConnections;
	UniqueIdentifierGenerator<int32_t> uidGen;
	
	std::map<int32_t, RPCPeerConnection*> connections;//id -> connection
	std::list<RPCPeerConnection*> newConnections;
	
	std::unordered_set<std::string> ownServices;
	
	std::list<std::function<void(const std::vector<std::string>&)>> peerServiceReceivers;
	std::vector<std::string> remoteServices;
	
	enum CONST_IDS{
		GET_PEER_SERVICES = 1 << 30
	};
	
	~RPCProxyPeerPrivate(){
		delete proxyRPC;
		for(auto it = connections.begin(); it != connections.end(); ++it){
			delete it->second;
		}
	}
	
	void OnProcedureResult(IRPCValue* results, uint32_t id){
		if(id==GET_PEER_SERVICES){
			remoteServices = createNativeValue<std::vector<std::string>>(results);
			for(auto& f : peerServiceReceivers){
				f(remoteServices);
			}
			peerServiceReceivers.clear();
		}else{
			auto it = pendingConnections.find(id);
			if(it!=pendingConnections.end()){
				int32_t trueConnID = createNativeValue<int32_t>(results);
				connections[trueConnID] = it->second;
				it->second->OnTrulyConnected(trueConnID);
				pendingConnections.erase(it);
				uidGen.returnId(id);
			}
		}
		delete results;
	}
	
	virtual void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		//not possible for pending connections
		delete errorData;
	}
	
};

RPCProxyPeer::RPCProxyPeer(IRPC* proxyRPC){
	p = new RPCProxyPeerPrivate();
	p->proxyRPC = proxyRPC;
	
	p->proxyFunctions["rc:connectPeer"] = LambdaCallReceiver([this](const std::string& procedure, const std::vector<IRPCValue*>& values){//called when an external peer wants to connect
		std::string serviceName = createNativeValue<std::string>(values[0]);
		int32_t id = createNativeValue<int32_t>(values[1]);
		if(p->ownServices.find(serviceName)!=p->ownServices.end()){//only allow if service offered
			RPCPeerConnection* c = new RPCPeerConnection(this, serviceName, id);
			p->connections[id] = c;
			p->newConnections.push_back(c);
		}else{
			p->proxyRPC->callRemoteProcedure("rc:disconnectPeer", std::vector<IRPCValue*>{createRPCValue(id)});
		}
		return (IRPCValue*)NULL;
	});
	
	p->proxyFunctions["rc:disconnectPeer"] = LambdaCallReceiver([this](const std::string& procedure, const std::vector<IRPCValue*>& values){
		int32_t id = createNativeValue<int32_t>(values[0]);
		auto it = p->connections.find(id);
		if(it!=p->connections.end()){
			it->second->disconnectSilent();
			p->connections.erase(it);
		}
		return (IRPCValue*)NULL;
	});
	
	p->proxyFunctions["rc:call"] = LambdaCallReceiver([this](const std::string& procedure, const std::vector<IRPCValue*>& values){
		int32_t id = createNativeValue<int32_t>(values[0]);
		std::string proc = createNativeValue<std::string>(values[1]);
		ArrayValue* vals = (ArrayValue*)values[2];
		auto it = p->connections.find(id);
		if(it!=p->connections.end()){
			return it->second->callLocally(proc, vals->values);
		}
		return (IRPCValue*)NULL;
	});
	
	p->proxyFunctions["rc:return"] = LambdaCallReceiver([this](const std::string& procedure, const std::vector<IRPCValue*>& values){
		return (IRPCValue*)NULL;//TODO find call based on call id and call local caller callback
	});
	
	for(auto it = p->proxyFunctions.begin(); it!=p->proxyFunctions.end(); ++it){
		proxyRPC->registerCallReceiver(it->first, &(it->second));
	}
}
	
RPCProxyPeer::~RPCProxyPeer(){
	for(auto it = p->proxyFunctions.begin(); it!=p->proxyFunctions.end(); ++it){
		p->proxyRPC->unregisterCallReceiver(it->first);
	}
	delete p;
}

void RPCProxyPeer::registerPeerService(const std::string& name){
	p->ownServices.insert(name);
	p->proxyRPC->callRemoteProcedure("rc:registerPeerService", std::vector<IRPCValue*>{createRPCValue(name)});
}

RPCPeerConnection* RPCProxyPeer::accept(){
	if(!p->newConnections.empty()){
		RPCPeerConnection* c = p->newConnections.front();
		p->newConnections.pop_front();
		return c;
	}
	return NULL;
}

RPCPeerConnection* RPCProxyPeer::connectToPeer(const std::string& serviceName){
	int32_t id = p->uidGen.getUniqueId();
	p->proxyRPC->callRemoteProcedure("rc:connectToPeerService", std::vector<IRPCValue*>{createRPCValue(serviceName)}, p, id);
	RPCPeerConnection* res = new RPCPeerConnection(this, serviceName);
	p->pendingConnections[id] = res;
	return res;
}

void RPCProxyPeer::requestServiceNames(const std::function<void(const std::vector<std::string>&)>& onReceivedCallback){
	p->proxyRPC->callRemoteProcedure("rc:getPeerServices", std::vector<IRPCValue*>{}, p, RPCProxyPeerPrivate::GET_PEER_SERVICES);
	p->peerServiceReceivers.push_back(onReceivedCallback);
}

const std::vector<std::string>& RPCProxyPeer::getServiceNames(){
	return p->remoteServices;
}

void RPCProxyPeer::update(){
	p->proxyRPC->update();
}
