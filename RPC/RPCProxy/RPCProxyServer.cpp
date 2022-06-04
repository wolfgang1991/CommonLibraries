#include "RPCProxyServer.h"

#include <UniqueIdentifierGenerator.h>

#include <iostream>

class RPCProxyServerPrivate : public IRemoteProcedureCaller{
	
	public:
	
	struct Peer{
		IRPCClient* client;
		std::unordered_map<std::string, LambdaCallReceiver> proxyFunctions;
		std::unordered_set<std::string> services;
		std::list<int32_t> connections;
		
		Peer(IRPCClient* client):client(client){}
		
		~Peer(){
			for(auto it = proxyFunctions.begin(); it!=proxyFunctions.end(); ++it){
				client->unregisterCallReceiver(it->first);
			}
			delete client;
		}
	};
	
	std::list<Peer> peers;
	std::unordered_map<std::string, Peer*> service2Peer;
	std::unordered_set<std::string> services;
	
	struct Connection{
		Peer* first;
		Peer* second;
		std::list<int32_t> calls;
	};
	
	UniqueIdentifierGenerator<int32_t> connIDGenerator;
	std::map<int32_t, Connection> connections;
	
	bool removeConnection(int32_t id){
		auto it = connections.find(id);
		if(it!=connections.end()){
			for(int i=0; i<2; i++){
				Peer* p = i==0?(it->second.first):(it->second.second);
				if(p->client->isConnected()){
					p->client->callRemoteProcedure("rc:disconnectPeer", std::vector<IRPCValue*>{createRPCValue(id)});
				}
			}
			connIDGenerator.returnId(id);
			for(int32_t callID : it->second.calls){
				calls.erase(callID);
				callIDGenerator.returnId(callID);
			}
			connections.erase(it);
			return true;
		}
		return false;
	}
	
	UniqueIdentifierGenerator<int32_t> callIDGenerator;
	std::map<int32_t, Peer*> calls;//peer is the one who receives the return value
	
	void OnProcedureResult(IRPCValue* results, uint32_t id){	
		auto it = calls.find(id);
		if(it!=calls.end()){
			it->second->client->callRemoteProcedure("rc:return", std::vector<IRPCValue*>{createRPCValue(id), results});
			calls.erase(id);
			callIDGenerator.returnId(id);
		}else{
			delete results;
		}
	}
	
	void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		delete errorData;//TODO
	}
	
};

RPCProxyServer::RPCProxyServer(){
	p = new RPCProxyServerPrivate();
}
	
RPCProxyServer::~RPCProxyServer(){
	delete p;
}
	
void RPCProxyServer::addPeer(IRPCClient* rpc){
	p->peers.emplace_back(rpc);//rpc gets deleted in push_back, therefore emplace
	RPCProxyServerPrivate::Peer* peer = &(p->peers.back());
	
	peer->proxyFunctions["rc:registerPeerService"] = LambdaCallReceiver([this, peer](const std::string& procedure, const std::vector<IRPCValue*>& values){//register a peer service for connection by other peer (string -> bool)
		if(values.size()==1 && values[0]->getType()==IRPCValue::STRING){//check, because this is the server
			std::string& s = ((StringValue*)(values[0]))->value;
			peer->services.insert(s);
			p->service2Peer[s] = peer;
			p->services.insert(s);
			return createRPCValue(true);
		}
		return createRPCValue(false);
	});
	
	peer->proxyFunctions["rc:getPeerServices"] = LambdaCallReceiver([this](const std::string& procedure, const std::vector<IRPCValue*>& values){//returns a list of registered peer services ( -> array of strings)
		return createRPCValue(p->services);
	});
	
	peer->proxyFunctions["rc:connectToPeerService"] = LambdaCallReceiver([this, peer](const std::string& procedure, const std::vector<IRPCValue*>& values){//returns a identifier or -1 in case of error (string -> int)
		if(values.size()==1 && values[0]->getType()==IRPCValue::STRING){//check, because this is the server
			std::string& s = ((StringValue*)(values[0]))->value;
			auto it = p->service2Peer.find(s);
			if(it!=p->service2Peer.end()){
				int32_t id = p->connIDGenerator.getUniqueId();
				p->connections[id] = RPCProxyServerPrivate::Connection{peer, it->second};
				peer->connections.push_back(id);
				it->second->connections.push_back(id);
				it->second->client->callRemoteProcedure("rc:connectPeer", std::vector<IRPCValue*>{createRPCValue(s), createRPCValue(id)});
				return createRPCValue(id);
			}
		}
		return createRPCValue(int32_t(-1));
	});
	
	peer->proxyFunctions["rc:disconnectPeer"] = LambdaCallReceiver([this, peer](const std::string& procedure, const std::vector<IRPCValue*>& values){//returns true if successful (int -> bool)
		if(values.size()==1 && values[0]->getType()==IRPCValue::INTEGER){//check, because this is the server
			return createRPCValue(p->removeConnection(createNativeValue<int32_t>(values[0])));//TODO only allow if peer is part of the connection
		}
		return createRPCValue(false);
	});
	
	peer->proxyFunctions["rc:call"] = LambdaCallReceiver([this, peer](const std::string& procedure, const std::vector<IRPCValue*>& values){//forwards a procedure call to the other peer related with the connection id (int connection id, string procname, array parameters -> int call id, -1 if error)
		if(values.size()==3 && values[0]->getType()==IRPCValue::INTEGER && values[1]->getType()==IRPCValue::STRING && values[2]->getType()==IRPCValue::ARRAY){//check, because this is the server
			auto it = p->connections.find(createNativeValue<int32_t>(values[0]));
			if(it!=p->connections.end()){
				RPCProxyServerPrivate::Peer* target = peer==it->second.first?(it->second.second):(it->second.first);
				int32_t callID = p->callIDGenerator.getUniqueId();
				p->calls[callID] = target;
				IRPCValue* connID = createRPCValue(it->first);
				target->client->callRemoteProcedure("rc:call", std::vector<IRPCValue*>{connID, values[1], values[2]}, p, callID, false);//don't delete (false) because values are members of the input values vector
				delete connID;
				return createRPCValue(callID);
			}
		}
		return createRPCValue(-1);
	});
	
	for(auto it = peer->proxyFunctions.begin(); it!=peer->proxyFunctions.end(); ++it){
		rpc->registerCallReceiver(it->first, &(it->second));
	}
}

void RPCProxyServer::update(){
	auto it = p->peers.begin();
	while(it != p->peers.end()){
		IRPCClient* client = it->client;
		if(client->isConnected()){
			client->update();
			++it;
		}else{
			for(const std::string& s : it->services){
				auto it = p->service2Peer.find(s);
				if(it!=p->service2Peer.end()){
					p->service2Peer.erase(it);
					p->services.erase(s);
				}
			}
			for(int32_t id : it->connections){
				p->removeConnection(id);
			}
			it = p->peers.erase(it);
		}
	}
}
