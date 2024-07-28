#include "StreamProxy.h"
#include "ZSocket.h"
#include "SSLSocket.h"
#include "SimpleSockets.h"

#include <memory>
#include <thread>
#include <atomic>
#include <list>
#include <cstdint>

struct ControlClient : public IRemoteProcedureCaller, IRemoteProcedureCallReceiver{

	//constant
	const IPv6Address address;
	
	//main thread only
	std::unique_ptr<std::thread> t;
	StreamProxyPrivate* p;
	
	//exchange
	std::atomic<uint8_t> state;//0: running, 1: must exit, 2: thread done
	
	//Thread only
	IPv6TCPSocket* control;
	SSLSocket* ssl;
	ZSocket* z;
	bool loginSuccess;
	std::unordered_set<std::string> authenticatedServices;//authentications prevail if service reconnects which is ok
	
	ControlClient(StreamProxyPrivate* p, IPv6TCPSocket* control, const IPv6Address& address);
	
	~ControlClient();
	
	void OnProcedureResult(IRPCValue* results, uint32_t id);
	
	void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id);

	IRPCValue* callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values);
	
};

class StreamProxyPrivate{
	
	public:
	
	//constant data
	SSLContext sslContext;
	const uint32_t pingTimeout;
	const std::string password;
	const uint16_t controlPort;
	const uint16_t dataPort;
	const uint16_t streamPort;
	
	//main thread only
	IPv6TCPSocket control;
	IPv6TCPSocket data;
	IPv4TCPSocket stream;
	
	std::list<std::shared_ptr<ControlClient>> controlClients;
	
	StreamProxyPrivate(const std::string password, uint16_t controlPort, uint16_t dataPort, uint16_t streamPort, uint32_t pingTimeout):sslContext(SSLContext::SERVER),pingTimeout(pingTimeout),password(password),controlPort(controlPort),dataPort(dataPort),streamPort(streamPort){
		control.bind(controlPort);
		control.listen();
		data.bind(dataPort);
		data.listen();
		stream.bind(streamPort);
		stream.listen();
	}
	
	void update(){
		//new control clients
		IPv6Address address;
		IPv6TCPSocket* newControl = control.accept(0, address);
		if(newControl!=NULL){
			controlClients.emplace_back(std::make_shared<ControlClient>(this, newControl, address));
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
		//TODO stream, data update
	}
	
	//! returns true if successful, may be called from any thread
	bool authenticate(const std::string& service, const std::string& password){
		return false;//TODO
	}
	
	//! can be called from any thread
	std::vector<std::string> createServiceList(){
		std::vector<std::string> res;
		//TODO
		return res;
	}
	
	//! can be called from any thread
	uint16_t addPendingConnection(const std::string& service, const IPv6Address& address){
		//TODO
		return dataPort;
	}
	
	//! can be called from any thread
	uint16_t addMCStreamConnection(const std::string& service, const std::string& streamName, const IPv6Address& address){
		//TODO
		return streamPort;
	}
	
	//! returns true if successful, may be called from any thread
	bool addService(const IPv6Address& address, const StreamProxyAPI::ServiceSpec& spec){
		//TODO
		return false;
	}
	
};

ControlClient::ControlClient(StreamProxyPrivate* p, IPv6TCPSocket* control, const IPv6Address& address):address(address),p(p),control(control){
	state = 0;
	loginSuccess = false;
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

void ControlClient::OnProcedureResult(IRPCValue* results, uint32_t id){
	//TODO
	delete results;
}

void ControlClient::OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
	//TODO
	delete errorData;
}

IRPCValue* ControlClient::callProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values){
	//check signatures since server is accessible from public internet:
	static const std::vector<IRPCValue::Type> loginSignature{IRPCValue::STRING};
	static const std::vector<IRPCValue::Type> authenticateSignature{IRPCValue::STRING, IRPCValue::STRING};
	static const std::vector<IRPCValue::Type> getServiceListSignature{};
	static const std::vector<IRPCValue::Type> connectTCPSignature{IRPCValue::STRING};
	static const std::vector<IRPCValue::Type> subscribeToMCStreamSignature{IRPCValue::STRING, IRPCValue::STRING};
	static const std::vector<IRPCValue::Type> registerServiceSignature{IRPCValue::OBJECT};
	if(procedure=="login" && hasValidRPCSignature(values, loginSignature)){
		loginSuccess = ((StringValue*)(values[0]))->value == p->password;
		return createRPCValue<bool>(loginSuccess);
	}else if(procedure=="authenticate" && hasValidRPCSignature(values, authenticateSignature)){
		if(loginSuccess){
			const std::string& service = ((StringValue*)(values[0]))->value;
			const std::string& password = ((StringValue*)(values[1]))->value;
			if(p->authenticate(service, password)){
				authenticatedServices.insert(service);
				return createRPCValue<bool>(true);
			}
		}
		return createRPCValue<bool>(false);
	}else if(procedure=="getServiceList" && hasValidRPCSignature(values, getServiceListSignature)){
		if(loginSuccess){
			return createRPCValue(p->createServiceList());
		}
		return createRPCValue(std::vector<std::string>());//empty list in case of error
	}else if(procedure=="connectTCP" && hasValidRPCSignature(values, connectTCPSignature)){
		if(loginSuccess){
			const std::string& service = ((StringValue*)(values[0]))->value;
			if(authenticatedServices.find(service)!=authenticatedServices.end()){
				return createRPCValue<uint16_t>(p->addPendingConnection(service, address));
			}
		}
		return createRPCValue<uint16_t>(0);
	}else if(procedure=="subscribeToMCStream" && hasValidRPCSignature(values, subscribeToMCStreamSignature)){
		if(loginSuccess){
			const std::string& service = ((StringValue*)(values[0]))->value;
			if(authenticatedServices.find(service)!=authenticatedServices.end()){
				return createRPCValue<uint16_t>(p->addMCStreamConnection(service, ((StringValue*)(values[1]))->value, address));
			}
		}
		return createRPCValue<uint16_t>(0);
	}else if(procedure=="registerService" && hasValidRPCSignature(values, registerServiceSignature)){//check object signature not required due to FILL_NATIVE_FIELD_IF_AVAILABLE
		if(loginSuccess){
			return createRPCValue<bool>(p->addService(address, createNativeValue<ServiceSpec>(values[0])));
		}
		return createRPCValue<bool>(false);
	}
	return NULL;
}

StreamProxy::StreamProxy(const std::string password, uint16_t controlPort, uint16_t dataPort, uint16_t streamPort, uint32_t pingTimeout){
	p = new StreamProxyPrivate(password, controlPort, dataPort, streamPort, pingTimeout);
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
