#include "StreamProxyClient.h"

#include <ProcedureCallAdapter.h>
#include <SSLSocket.h>
#include <SimpleSockets.h>
#include <JSONRPC2Client.h>
#include <timing.h>

#include <iostream>
#include <unordered_map>

struct StreamProxyClientPrivate{
	
	uint32_t connectTimeout;
	uint32_t pingTimeout;
	
	SSLContext c;
	ASocket* tcp2proxy;
	SSLSocket* ssl2proxy;
	JSONRPC2Client* client;
	
	std::list<IIPAddress*> addressList;
	
	std::unordered_map<std::string, std::function<void(const std::string&, ASocket*)> > service2newTCPCallback;
	
	std::shared_ptr<X509Cert> publicCert;
	
	StreamProxyClientPrivate(uint32_t connectTimeout, uint32_t pingTimeout):connectTimeout(connectTimeout),pingTimeout(pingTimeout),c(SSLContext::CLIENT),tcp2proxy(NULL),ssl2proxy(NULL){}
	
	~StreamProxyClientPrivate(){
		delete ssl2proxy;
	}
	
};

StreamProxyClient::StreamProxyClient(uint32_t connectTimeout, uint32_t pingTimeout){
	p = new StreamProxyClientPrivate(connectTimeout, pingTimeout);
}

StreamProxyClient::~StreamProxyClient(){
	delete p;
}

bool StreamProxyClient::connect(const std::list<IIPAddress*>& addressList, std::shared_ptr<X509Cert> publicCert){
	p->publicCert = publicCert;
	disconnect();
	deleteAddressList(p->addressList);
	p->addressList = copyAddressList(addressList);
	ASocket* s = connectSocketForAddressList(addressList, p->connectTimeout);
	if(s){
		p->tcp2proxy = s;//even if it is null, to abort previous connection
		p->ssl2proxy = new SSLSocket(&(p->c), s, true);
		p->client = new JSONRPC2Client();
		p->client->useSocket(p->ssl2proxy, p->pingTimeout, p->pingTimeout/3);
		if(p->publicCert && !p->ssl2proxy->isPeerCertificateEqual(*(p->publicCert))){
			disconnect();
			std::cerr << "Proxy server public certificate / key does not match." << std::endl;
			return false;
		}else if(!p->publicCert){
			std::cerr << "Warning: Encryption not reliable due to missing certificate / public key for proxy server." << std::endl;
		}
	}
	return s!=NULL;
}

void StreamProxyClient::disconnect(){
	delete p->client;
	p->client = NULL;
	p->ssl2proxy = NULL;
	p->tcp2proxy = NULL;
	p->service2newTCPCallback.clear();
}

bool StreamProxyClient::isConnected() const{
	if(p->client){return p->client->isConnected();}
	return false;
}

void StreamProxyClient::login(const std::string& password, const std::function<void(bool)>& OnLoginResult){
	if(p->client){
		StreamProxyAPI::login(p->client, password, new ProcedureCallAdapter([this, OnLoginResult](IRPCValue* results, uint32_t id){
			bool success = createNativeValue<bool>(results);
			delete results;
			OnLoginResult(success);
		}));
	}else{
		OnLoginResult(false);
	}
}

void StreamProxyClient::registerService(const std::string& service, const std::string& password, const std::function<void(bool)>& OnRegisterServiceResult, const std::function<void(const std::string&, ASocket*)>& OnNewTCPClient){
	if(p->client){
		p->service2newTCPCallback[service] = OnNewTCPClient;
		StreamProxyAPI::registerService(p->client, StreamProxyAPI::ServiceSpec{service, password}, new ProcedureCallAdapter([this, OnRegisterServiceResult](IRPCValue* results, uint32_t id){
			bool success = createNativeValue<bool>(results);
			delete results;
			OnRegisterServiceResult(success);
		}));
	}else{
		OnRegisterServiceResult(false);
	}
}

void StreamProxyClient::authenticate(const std::string& service, const std::string& password, const std::function<void(const std::string&, bool)>& OnAuthenticateResult){
	if(p->client){
		StreamProxyAPI::authenticate(p->client, service, password, new ProcedureCallAdapter([this, OnAuthenticateResult, service](IRPCValue* results, uint32_t id){
			bool success = createNativeValue<bool>(results);
			delete results;
			OnAuthenticateResult(service, success);
		}));
	}else{
		OnAuthenticateResult(service, false);
	}
}

ASocket* StreamProxyClient::connectTCP(const std::string& service){
	ASocket* s = NULL;
	if(p->client){
		bool running = true;
		StreamProxyAPI::connectTCP(p->client, service, new ProcedureCallAdapter([this, &running, &s](IRPCValue* results, uint32_t id){
			uint16_t port = createNativeValue<uint16_t>(results);
			delete results;
			if(port!=0){
				std::list<IIPAddress*> l = copyAddressList(p->addressList);
				for(IIPAddress* a : l){a->setPort(port);}
				s = connectSocketForAddressList(l, p->connectTimeout);
			}
			running = false;
		}));
		while(running){//block for rpc return, then tcp connect is always blocking
			p->client->update();
			delay(10);
		}
	}
	return s;
}

void StreamProxyClient::getServiceList(const std::function<void(const std::vector<std::string>&)>& OnServiceListResult){
	//TODO
}

void StreamProxyClient::update(){
	if(p->client){p->client->update();}
}
