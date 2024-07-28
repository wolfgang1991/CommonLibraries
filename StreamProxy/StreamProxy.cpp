#include "StreamProxy.h"
#include "ZSocket.h"
#include "SSLSocket.h"
#include "SimpleSockets.h"

#include <memory>
#include <thread>
#include <atomic>
#include <list>
#include <cstdint>

struct ControlClient : public IRemoteProcedureCaller, IRemoteProcedureCallReceiver{{
	
	//main thread only
	std::unique_ptr<std::thread>
	StreamProxyPrivate* p;
	
	//exchange
	std::atomic<uint8_t> state;//0: running, 1: must exit, 2: thread done
	
	//Thread only
	IPv6TCPSocket* control;
	SSLSocket* ssl;
	ZSocket* z;
	
	ControlClient(StreamProxyPrivate* p, IPv6TCPSocket* control);
	
	~ControlClient();
	
	void OnProcedureResult(IRPCValue* results, uint32_t id){
		//TODO
		delete results;
	}
	
	void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
		//TODO
		delete errorData;
	}

	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
//		client.registerCallReceiver("login", this);//login @ proxy to get service list & register service
//		client.registerCallReceiver("authenticate", this);//authenticate for service
//		client.registerCallReceiver("getServiceList", this);
//		client.registerCallReceiver("connectTCP", this);//connect to service @ specific port
//		client.registerCallReceiver("subscribeToMCStream", this);//subscribes to a multicast stream of a service (service name, stream name)
//		client.registerCallReceiver("registerService", this);
		//check signatures since server is accessible from public internet:
		static const std::vector<IRPCValue::Type> loginSignature{IRPCValue::STRING};
		if(procedure=="login" && hasValidRPCSignature(values, loginSignature)){
			//TODO
		}
		//TODO
		return NULL;
	}
	
};

class StreamProxyPrivate{
	
	public:
	
	SSLContext sslContext;
	const uint32_t pingTimeout;
	
	IPv6TCPSocket control;
	IPv6TCPSocket data;
	
	std::list<std::shared_ptr<ControlClient>> controlClients;
	
	StreamProxyPrivate(uint16_t controlPort, uint16_t dataPort, uint32_t pingTimeout):sslContext(SSLContext::SERVER),pingTimeout(pingTimeout){
		control.bind(controlPort);
		control.listen();
		data.bind(dataPort);
		data.listen();
	}
	
	void update(){
		//new control clients
		IPv6TCPSocket* newControl = control.accept();
		if(newControl!=NULL){
			controlClients.emplace_back(std::make_shared<ControlClient>(this, newControl));
		}
		//update control clients
		auto cit = controlClients.begin();
		while(it!=controlClients.end()){
			if(c->state==2){
				it = controlClients.erase(it);
			}else{
				++it;
			}
		}
	}
	
};

ControlClient::ControlClient(StreamProxyPrivate* p, IPv6TCPSocket* control):p(p),control(control){
	state = 0;
	t = std::unique_ptr<std::thread>(new std::thread([this](){
		bool running = true;
		ssl = new SSLSocket(&(p->sslContext), control, true);
		z = new ZSocket(ssl);
		JSONRPC2Client client;
		client.useSocket(z, p->pingTimeout, PING_DISABLE_SEND_PERIOD);
		client.registerCallReceiver("login", this);//login @ proxy to get service list & register service
		client.registerCallReceiver("authenticate", this);//authenticate for service
		client.registerCallReceiver("getServiceList", this);
		client.registerCallReceiver("connectTCP", this);//connect to service @ specific port
		client.registerCallReceiver("subscribeToMCStream", this);//subscribes to a multicast stream of a service (service name, stream name)
		client.registerCallReceiver("registerService", this);
		while(running){
			running = state != 1;
			client.update();
			running = running && client.isConnected();//ping timeout => done
			delay(10);//long delay since control does not need speed
		}
		state = 2;
	}));
}

ControlClient::~ControlClient(){
	state = 1;
	t->join();
	//z & ssl & control is deleted by json rpc client in other thread
}

StreamProxy::StreamProxy(uint16_t controlPort, uint16_t dataPort, uint32_t pingTimeout){
	p = new StreamProxyPrivate(controlPort, dataPort, pingTimeout);
}
	
StreamProxy::~StreamProxy(){
	delete p;
}

bool StreamProxy::usePrivateKey(const std::vector<char>& data){
	return p->sslContext.usePrivateKey(data);
}

bool StreamProxy::useCertificate(const std::vector<char>& data){
	return p->sslContext.useCertificate(data);
}

bool StreamProxy::usePrivateKeyFromFile(const std::string& path){
	return p->sslContext.usePrivateKeyFromFile(path);
}

bool StreamProxy::useCertificateFromFile(const std::string& path){
	return p->sslContext.useCertificateFromFile(path);
}

void StreamProxy::update(){
	p->update();
}
