#include "StreamProxy.h"
#include "StreamProxyAPI.h"

#include <ZSocket.h>
#include <SSLSocket.h>
#include <SimpleSockets.h>
#include <JSONRPC2Client.h>

#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <list>
#include <cstdint>
#include <iostream>

struct ControlClient : public IRemoteProcedureCaller, IRemoteProcedureCallReceiver{

	//constant
	const IPv6Address address;
	
	//main thread only
	std::unique_ptr<std::thread> t;
	StreamProxyPrivate* p;
	
	//exchange
	std::atomic<uint8_t> state;//0: running, 1: must exit, 2: thread done
	
	std::mutex mPending;
	std::list<std::string> pendingServices;
	
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

struct TCPConnection{
	
	//main thread only
	std::unique_ptr<std::thread> t;
	StreamProxyPrivate* p;
	
	//exchange
	std::atomic<uint8_t> state;//0: running, 1: must exit, 2: thread done
	
	//Thread only
	std::shared_ptr<IPv6TCPSocket> firstSocket;
	std::shared_ptr<IPv6TCPSocket> secondSocket;
	
	static constexpr uint32_t size = 1024*1024;
	char buf[size];
	
	TCPConnection(StreamProxyPrivate* p, std::shared_ptr<IPv6TCPSocket> firstSocket, std::shared_ptr<IPv6TCPSocket> secondSocket);
	
};

class StreamProxyPrivate{
	
	public:
	
	struct PendingTCPConnection{
		double startTime;
		IPv6Address first;
		IPv6Address second;
		std::shared_ptr<IPv6TCPSocket> firstSocket;
		std::shared_ptr<IPv6TCPSocket> secondSocket;
		
		PendingTCPConnection(const IPv6Address& first, const IPv6Address& second):first(first),second(second),firstSocket(std::shared_ptr<IPv6TCPSocket>(nullptr)),secondSocket(std::shared_ptr<IPv6TCPSocket>(nullptr)){
			this->first.setPort(0);
			this->second.setPort(0);
			startTime = getSecs();
		}
	};
	
	struct Service{
		IPv6Address address;
		std::string password;
		ControlClient* client;
	};
	
	std::mutex mService;
	std::unordered_map<std::string, std::shared_ptr<Service>> services;
	std::map<ControlClient*, std::list<std::string>> controlClient2Services;
	
	std::mutex mPendingConnections;
	std::list<PendingTCPConnection> pendingTCP;
	
	//constant data
	SSLContext sslContext;
	const uint32_t pingTimeout;//ms
	const double dataExchangeTimeout;//s
	const std::string password;
	const uint16_t controlPort;
	const uint16_t dataPort;
	
	//main thread only
	IPv6TCPSocket control;
	IPv6TCPSocket data;
	
	std::list<std::shared_ptr<ControlClient>> controlClients;
	std::list<std::shared_ptr<TCPConnection>> dataClients;
	
	StreamProxyPrivate(const std::string password, uint16_t controlPort, uint16_t dataPort, uint32_t pingTimeout, uint32_t dataExchangeTimeout):sslContext(SSLContext::SERVER),pingTimeout(pingTimeout),dataExchangeTimeout(dataExchangeTimeout/1000.0),password(password),controlPort(controlPort),dataPort(dataPort){
		control.bind(controlPort);
		control.listen(10);
		data.bind(dataPort);
		data.listen(10);
	}
	
	void update(){
		//new control clients
		IPv6Address address;
		IPv6TCPSocket* newControl = control.accept(0, &address);
		if(newControl!=NULL){
			controlClients.emplace_back(std::make_shared<ControlClient>(this, newControl, address));
		}
		//update control clients
		auto cit = controlClients.begin();
		while(cit!=controlClients.end()){
			if((*cit)->state==2){
				{
					std::lock_guard<std::mutex> lock(mService);
					auto sit = controlClient2Services.find(cit->get());
					if(sit!=controlClient2Services.end()){
						for(std::string& s : sit->second){
							services.erase(s);
						}
					}
				}
				cit = controlClients.erase(cit);
			}else{
				++cit;
			}
		}
		//new data clients
		IPv6Address dataAddress;
		IPv6TCPSocket* newData = data.accept(0, &dataAddress);
		if(newData!=NULL){
			dataAddress.setPort(0);//to compare
			std::lock_guard<std::mutex> lock(mPendingConnections);
			auto it = pendingTCP.begin();
			bool notUsed = true;
			while(notUsed && it!=pendingTCP.end()){
				if(!it->firstSocket && it->first==dataAddress){
					it->firstSocket = std::shared_ptr<IPv6TCPSocket>(newData);
					notUsed = false;
				}else if(!it->secondSocket && it->second==dataAddress){
					it->secondSocket = std::shared_ptr<IPv6TCPSocket>(newData);
					notUsed = false;
				}
				if(it->firstSocket && it->secondSocket){
					dataClients.emplace_back(std::shared_ptr<TCPConnection>(new TCPConnection(this, it->firstSocket, it->secondSocket)));
					it = pendingTCP.erase(it);
				}else{
					++it;
				}
			}
		}
		//update data clients
		auto dit = dataClients.begin();
		while(dit!=dataClients.end()){
			if((*dit)->state==2){
				dit = dataClients.erase(dit);
			}else{
				++dit;
			}
		}
		//timeout for pending connections
		{
			double t = getSecs();
			std::lock_guard<std::mutex> lock(mPendingConnections);
			auto it = pendingTCP.begin();
			while(it!=pendingTCP.end()){
				if(t-it->startTime>dataExchangeTimeout){
					std::cout << "Connection timeout." << std::endl;
					it = pendingTCP.erase(it);
				}else{
					++it;
				}
			}
		}
	}
	
	//! returns true if successful, may be called from any thread
	bool authenticate(const std::string& service, const std::string& password){
		std::lock_guard<std::mutex> lock(mService);
		auto it = services.find(service);
		if(it!=services.end()){
			return it->second->password==password;
		}
		return false;
	}
	
	//! can be called from any thread
	std::vector<std::string> createServiceList(){
		std::vector<std::string> res;
		{
			std::lock_guard<std::mutex> lock(mService);
			res.reserve(services.size());
			for(auto it=services.begin(); it!=services.end(); ++it){
				res.emplace_back(it->first);
			}
		}
		return res;
	}
	
	//! can be called from any thread, 0 if unsuccessful
	uint16_t addPendingConnection(const std::string& service, const IPv6Address& address){
		std::lock_guard<std::mutex> lock(mService);
		auto it = services.find(service);
		if(it!=services.end()){
			{
				std::lock_guard<std::mutex> lock(mPendingConnections);
				pendingTCP.emplace_back(PendingTCPConnection(address, it->second->address));
			}
			{
				std::lock_guard<std::mutex> lock(it->second->client->mPending);
				it->second->client->pendingServices.emplace_back(service);
			}
			return dataPort;
		}
		return 0;
	}
	
	/*
	//! can be called from any thread
	uint16_t addMCStreamConnection(const std::string& service, const std::string& streamName, const IPv6Address& address){
		//TODO
		return streamPort;
	}
	*/
	
	//! overrides existing service, may be called from any thread
	void addService(ControlClient* client, const IPv6Address& address, StreamProxyAPI::ServiceSpec& spec){
		std::shared_ptr<Service> s = std::shared_ptr<Service>(new Service{address, std::move(spec.password), client});
		{
			std::lock_guard<std::mutex> lock(mService);
			services[spec.name] = s;
			controlClient2Services[client].emplace_back(spec.name);
		}
	}
	
};

TCPConnection::TCPConnection(StreamProxyPrivate* p, std::shared_ptr<IPv6TCPSocket> firstSocket, std::shared_ptr<IPv6TCPSocket> secondSocket):p(p){
	this->firstSocket = firstSocket;
	this->secondSocket = secondSocket;
	state = 0;
	t = std::unique_ptr<std::thread>(new std::thread([this](){
		bool running = true;
		double lastReceiveTime = getSecs();
		while(running){
			double t = getSecs();
			running = state != 1 && t-lastReceiveTime<this->p->dataExchangeTimeout;
			bool anyReceived = false;
			uint32_t received = this->firstSocket->recv(buf, size, false);
			if(received>0){
				anyReceived = true;
				this->secondSocket->send(buf, received);
			}
			received = this->secondSocket->recv(buf, size, false);
			if(received>0){
				anyReceived = true;
				this->firstSocket->send(buf, received);
			}
			if(anyReceived){
				lastReceiveTime = t;
			}else{
				delay(2);
			}
		}
		std::cout << "TCP connection terminated." << std::endl;
		state = 2;
	}));
}

ControlClient::ControlClient(StreamProxyPrivate* p, IPv6TCPSocket* control, const IPv6Address& address):address(address),p(p),control(control){
	state = 0;
	loginSuccess = false;
	t = std::unique_ptr<std::thread>(new std::thread([this, control](){
		bool running = true;
		ssl = new SSLSocket(&(this->p->sslContext), control, true);
		z = new ZSocket(ssl);
		JSONRPC2Client client;
		client.useSocket(z, this->p->pingTimeout, PING_DISABLE_SEND_PERIOD);
		client.registerCallReceiver("login", this);//login @ proxy to get service list & register service
		client.registerCallReceiver("authenticate", this);//authenticate for service
		client.registerCallReceiver("getServiceList", this);
		client.registerCallReceiver("connectTCP", this);//connect to service @ specific port
//		client.registerCallReceiver("subscribeToMCStream", this);//subscribes to a multicast stream of a service (service name, stream name)
		client.registerCallReceiver("registerService", this);
		while(running){
			running = state != 1;
			client.update();
			running = running && client.isConnected();//ping timeout => done
			{
				std::lock_guard<std::mutex> lock(mPending);
				while(!pendingServices.empty()){
					StreamProxyAPI::OnNewTCPClient(&client, pendingServices.front(), this->p->dataPort, this);
					pendingServices.pop_front();
				}
			}
			delay(10);//long delay since control does not need speed
		}
		std::cout << "Control connection terminated." << std::endl;
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
//	static const std::vector<IRPCValue::Type> subscribeToMCStreamSignature{IRPCValue::STRING, IRPCValue::STRING};
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
		/*
	}else if(procedure=="subscribeToMCStream" && hasValidRPCSignature(values, subscribeToMCStreamSignature)){
		if(loginSuccess){
			const std::string& service = ((StringValue*)(values[0]))->value;
			if(authenticatedServices.find(service)!=authenticatedServices.end()){
				return createRPCValue<uint16_t>(p->addMCStreamConnection(service, ((StringValue*)(values[1]))->value, address));
			}
		}
		return createRPCValue<uint16_t>(0);
	*/
	}else if(procedure=="registerService" && hasValidRPCSignature(values, registerServiceSignature)){//check object signature not required due to FILL_NATIVE_FIELD_IF_AVAILABLE
		if(loginSuccess){
			StreamProxyAPI::ServiceSpec spec = createNativeValue<StreamProxyAPI::ServiceSpec>(values[0]);
			p->addService(this, address, spec);
		}
		return createRPCValue<bool>(loginSuccess);
	}
	return NULL;
}

StreamProxy::StreamProxy(const std::string password, uint16_t controlPort, uint16_t dataPort, uint32_t pingTimeout, uint32_t dataExchangeTimeout){
	p = new StreamProxyPrivate(password, controlPort, dataPort, pingTimeout, dataExchangeTimeout);
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
