#ifndef StreamProxyAPI_H_
#define StreamProxyAPI_H_

#include <IRPC.h>

namespace StreamProxyAPI{

constexpr uint32_t proxyPingTimeout = 4000;
constexpr uint32_t dataExchangeTimeout = 4000;

class ServiceSpec{
	
	public:
	
	std::string name;
	std::string password;
	
	CREATE_BEGIN(ServiceSpec)
		FILL_FIELD(name)
		FILL_FIELD(password)
	CREATE_END
	
	CREATE_NATIVE_BEGIN(ServiceSpec)
		FILL_NATIVE_FIELD_IF_AVAILABLE(name, std::string("noname"))
		FILL_NATIVE_FIELD_IF_AVAILABLE(password, std::string(""))
	CREATE_NATIVE_END
};

enum RemoteApiFunctions{
	//@Server:
	LOGIN,
	AUTHENTICATE,
	GETSERVICELIST,
	CONNECTTCP,
	REGISTERSERVICE,
	//@Client:
	ONNEWTCPCLIENT,
	REMOTE_API_FUNCTION_COUNT
};

//! true if successful
inline void login(IRPC* rpc, const std::string& password, IRemoteProcedureCaller* caller, uint32_t id = LOGIN){
	rpc->callRemoteProcedure("login", std::vector<IRPCValue*>{createRPCValue(password)}, caller, id);
}

//! true if successful
inline void authenticate(IRPC* rpc, const std::string& service, const std::string& password, IRemoteProcedureCaller* caller, uint32_t id = AUTHENTICATE){
	rpc->callRemoteProcedure("authenticate", std::vector<IRPCValue*>{createRPCValue(service), createRPCValue(password)}, caller, id);
}

//! only possible if login, returns list of service names
inline void getServiceList(IRPC* rpc, IRemoteProcedureCaller* caller, uint32_t id = GETSERVICELIST){
	rpc->callRemoteProcedure("getServiceList", std::vector<IRPCValue*>{}, caller, id);
}

//! initiates a tcp connection to a service, returns the port to connect (TLS over Z over TCP) or 0 if unsuccessful (e.g. no login, no authenticate)
inline void connectTCP(IRPC* rpc, const std::string& service, IRemoteProcedureCaller* caller, uint32_t id = CONNECTTCP){
	rpc->callRemoteProcedure("connectTCP", std::vector<IRPCValue*>{createRPCValue(service)}, caller, id);
}

//! registers a service if login done and no service with that name present, returns true if successful
inline void registerService(IRPC* rpc, const ServiceSpec& spec, IRemoteProcedureCaller* caller, uint32_t id = REGISTERSERVICE){
	rpc->callRemoteProcedure("registerService", std::vector<IRPCValue*>{createRPCValue(spec)}, caller, id);
}

//! called @ proxy client / server from other point of view to establish a new tcp connection
//! port: port to connect @ proxy (TLS over Z over TCP)
inline void OnNewTCPClient(IRPC* rpc, const std::string& service, uint16_t port, IRemoteProcedureCaller* caller, uint32_t id = ONNEWTCPCLIENT){
	rpc->callRemoteProcedure("OnNewTCPClient", std::vector<IRPCValue*>{createRPCValue(service), createRPCValue(port)}, caller, id);
}

}

#endif
