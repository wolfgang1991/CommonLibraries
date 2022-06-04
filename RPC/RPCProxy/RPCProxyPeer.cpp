#include "RPCProxyPeer.h"

#include <UniqueIdentifierGenerator.h>

#include <set>
#include <iostream>

class RPCPeerConnectionPrivate{

	public:
	
	RPCProxyPeer* peer;
	IRPCClient::ClientState state;
	std::unordered_map<std::string, IRemoteProcedureCallReceiver*> receivers;
	
};


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
	
	struct Call : public IRemoteProcedureCaller{
		
		RPCProxyPeerPrivate* p;
		int32_t callID;
		IRemoteProcedureCaller* caller;
		uint32_t callerSideID;
		
		Call(RPCProxyPeerPrivate* p, IRemoteProcedureCaller* caller, uint32_t callerSideID):p(p),callID(-1),caller(caller),callerSideID(callerSideID){}
		
		void OnProcedureResult(IRPCValue* results, uint32_t id){//only used for rc:call
			callID = createNativeValue<int32_t>(results);
			delete results;
			if(caller==NULL){
				p->calls.erase(this);
				delete this;
			}else if(callID<0){//error
				caller->OnProcedureError(-32000, "Error in remote proxy.", NULL, callerSideID);
				p->calls.erase(this);
				delete this;
			}else{
				p->callsWithID[callID] = this;
			}
		}
		
	};
	
	std::set<Call*> calls;
	std::map<int32_t, Call*> callsWithID;
	
	enum CONST_IDS{
		GET_PEER_SERVICES = 1 << 30
	};
	
	~RPCProxyPeerPrivate(){
		delete proxyRPC;
		for(auto it = connections.begin(); it != connections.end(); ++it){
			delete it->second;
		}
		for(Call* c : calls){
			delete c;
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

int32_t RPCPeerConnection::getConnectionID() const{
	return id;
}
		
const std::string& RPCPeerConnection::getServiceName() const{
	return serviceName;
}

RPCPeerConnection::RPCPeerConnection(RPCProxyPeer* peer, const std::string& serviceName, int32_t id):peer(peer),serviceName(serviceName),id(id){
	p = new RPCPeerConnectionPrivate();
	p->peer = peer;
	p->state = CONNECTED;
}

RPCPeerConnection::RPCPeerConnection(RPCProxyPeer* peer, const std::string& serviceName):peer(peer),serviceName(serviceName),id(-1){
	p = new RPCPeerConnectionPrivate();
	p->peer = peer;
	p->state = CONNECTING;
}

RPCPeerConnection::~RPCPeerConnection(){
	delete p;
}

void RPCPeerConnection::disconnectSilent(){
	id = -1;
	p->state = NOT_CONNECTED;
}

IRPCValue* RPCPeerConnection::callLocally(const std::string& procedure, const std::vector<IRPCValue*>& values){
	std::cout << "callLocally " << ((uint64_t)this) << " procedure: " << procedure << std::endl;
	auto it = p->receivers.find(procedure);
	if(it!=p->receivers.end()){
		return it->second->callProcedure(procedure, values);
	}
	std::cout << "p->receivers.size(): " << (p->receivers.size()) << std::endl;
	for(auto it = p->receivers.begin(); it != p->receivers.end(); ++it){std::cout << "receiver: " << (it->first) << std::endl;}
	return NULL;
}

void RPCPeerConnection::OnTrulyConnected(int32_t connID){
	id = connID;
	p->state = CONNECTED;
}

bool RPCPeerConnection::callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller, uint32_t id, bool deleteValues){
	bool res = false;
	if(p->state==CONNECTED){
		ArrayValue* params = new ArrayValue();
		params->values = values;
		RPCProxyPeerPrivate::Call* c = new RPCProxyPeerPrivate::Call(peer->p, caller, id);
		peer->p->calls.insert(c);
		res = peer->p->proxyRPC->callRemoteProcedure("rc:call", std::vector<IRPCValue*>{createRPCValue(this->id), createRPCValue(procedure), params}, c, 0, deleteValues);//call id here 0
	}else if(deleteValues){
		for(IRPCValue* v : values){delete v;}
	}
	return res;
}

void RPCPeerConnection::registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver){
	p->receivers[procedure] = receiver;
	std::cout << "registerCallReceiver " << ((uint64_t)this) << " p->receivers.size(): " << (p->receivers.size()) << std::endl;
	for(auto it = p->receivers.begin(); it != p->receivers.end(); ++it){std::cout << "receiver: " << (it->first) << std::endl;}
}

void RPCPeerConnection::unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver){
	auto it = p->receivers.find(procedure);
	if(it!=p->receivers.end() && it->second==receiver){
		p->receivers.erase(it);
	}
}

void RPCPeerConnection::removeProcedureCaller(IRemoteProcedureCaller* caller){
	for(RPCProxyPeerPrivate::Call* call : peer->p->calls){
		if(call->caller==caller){
			call->caller = NULL;
		}
	}
}

void RPCPeerConnection::update(){
	//already updated by callbacks from RPCProxyPeer
}

void RPCPeerConnection::connect(const IIPAddress& address, uint32_t pingSendPeriod, uint32_t pingTimeout, uint32_t connectTimeout, IMetaProtocolHandler* metaProtocolHandler){
	//not applicable
}

IRPCClient::ClientState RPCPeerConnection::getState() const{
	return p->state;
}

void RPCPeerConnection::disconnect(){
	if(p->state==CONNECTED){
		peer->p->proxyRPC->callRemoteProcedure("rc:disconnectPeer", std::vector<IRPCValue*>{createRPCValue(this->id)});
		disconnectSilent();
	}
}

void RPCPeerConnection::flush(){
	//not applicable
}

IRPCValue* moveToNewRPCValue(IRPCValue* value){
	static_assert(IRPCValue::TYPE_COUNT==8, "");
	auto type = value->getType();
	if(type==IRPCValue::NIL || type==IRPCValue::UNKNOWN){
		return new NULLValue();
	}else if(type==IRPCValue::BOOLEAN){
		return new BooleanValue(static_cast<BooleanValue*>(value)->value);
	}else if(type==IRPCValue::FLOAT){
		return new FloatValue(static_cast<FloatValue*>(value)->value);
	}else if(type==IRPCValue::INTEGER){
		return new IntegerValue(static_cast<IntegerValue*>(value)->value);
	}else if(type==IRPCValue::STRING){
		StringValue* s = new StringValue("");
		s->value = std::move(static_cast<StringValue*>(value)->value);
		return s;
	}else if(type==IRPCValue::ARRAY){
		ArrayValue* a =  new ArrayValue();
		a->values = std::move(static_cast<ArrayValue*>(value)->values);
		return a;
	}else{//OBJECT
		ObjectValue* o = new ObjectValue();
		o->values = std::move(static_cast<ObjectValue*>(value)->values);
		return o;
	}
}

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
			return it->second->callLocally(proc, vals->values);//TODO what if method not found?
		}
		return (IRPCValue*)NULL;
	});
	
	p->proxyFunctions["rc:return"] = LambdaCallReceiver([this](const std::string& procedure, const std::vector<IRPCValue*>& values){
		int32_t callID = createNativeValue<int32_t>(values[0]);
		IRPCValue* result = values[1];
		auto it = p->callsWithID.find(callID);
		if(it!=p->callsWithID.end()){
			RPCProxyPeerPrivate::Call* call = it->second;
			call->caller->OnProcedureResult(moveToNewRPCValue(result), call->callerSideID);
			delete call;
			p->calls.erase(call);
			p->callsWithID.erase(it);
		}
		return (IRPCValue*)NULL;
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
