#ifndef StreamProxyAPI_H_
#define StreamProxyAPI_H_

#include <IRPC.h>

namespace StreamProxyAPI{

/*
class MulticastStreamSpec{
	
	public:
	
	std::string name;
	std::string ipAddress;//! multicast ip address
	uint16_t port;
	
	CREATE_BEGIN(MulticastStreamSpec)
		FILL_FIELD(name)
		FILL_FIELD(ipAddress)
		FILL_FIELD(port)
	CREATE_END
	
	CREATE_NATIVE_BEGIN(MulticastStreamSpec)
		FILL_NATIVE_FIELD_IF_AVAILABLE(name, std::string("unspecified"))
		FILL_NATIVE_FIELD_IF_AVAILABLE(ipAddress, std::string("127.0.0.1"))
		FILL_NATIVE_FIELD_IF_AVAILABLE(port, 52964)
	CREATE_NATIVE_END
	
};
*/

class ServiceSpec{
	
	public:
	
	std::string name;
	std::string password;
	std::list<std::string> udpStreams;
	
	CREATE_BEGIN(ServiceSpec)
		FILL_FIELD(name)
		FILL_FIELD(password)
		FILL_FIELD(udpStreams)
	CREATE_END
	
	CREATE_NATIVE_BEGIN(ServiceSpec)
		FILL_NATIVE_FIELD_IF_AVAILABLE(name, std::string("noname"))
		FILL_NATIVE_FIELD_IF_AVAILABLE(password, std::string(""))
		FILL_NATIVE_FIELD_IF_AVAILABLE(udpStreams, std::list<std::string>())
	CREATE_NATIVE_END
};

enum RemoteApiFunctions{
	//@Server:
	LOGIN,
	AUTHENTICATE,
	GETSERVICELIST,
	CONNECTTCP,
/*	SUBSCRIBETOMCSTREAM, TODO UDP stream forwarding via layer on top*/
	REGISTERSERVICE,
	//@Client:
	ONNEWTCPCLIENT,
/*	SETMCSTREAMFORWARD,*/
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

/*//! subscribes to a multicast stream, returns the port to connect (TLS over Z over TCP) or 0 if unsuccessful*/
/*//! to avoid stream duplication an RemoteUDPStreamManager should run in the local netwok to retransmit via multicast*/
/*inline void subscribeToMCStream(IRPC* rpc, const std::string& service, const std::string& streamName, IRemoteProcedureCaller* caller, uint32_t id = SUBSCRIBETOMCSTREAM){*/
/*	rpc->callRemoteProcedure("subscribeToMCStream", std::vector<IRPCValue*>{createRPCValue(service), createRPCValue(streamName)}, caller, id);*/
/*}*/

//! registers a service if login done and no service with that name present, returns true if successful
inline void registerService(IRPC* rpc, const ServiceSpec& spec, IRemoteProcedureCaller* caller, uint32_t id = REGISTERSERVICE){
	rpc->callRemoteProcedure("registerService", std::vector<IRPCValue*>{createRPCValue(spec)}, caller, id);
}

//! called @ proxy client / server from other point of view to establish a new tcp connection
//! port: port to connect @ proxy (TLS over Z over TCP)
inline void OnNewTCPClient(IRPC* rpc, const std::string& service, uint16_t port, IRemoteProcedureCaller* caller, uint32_t id = ONNEWTCPCLIENT){
	rpc->callRemoteProcedure("OnNewTCPClient", std::vector<IRPCValue*>{createRPCValue(service), createRPCValue(port)}, caller, id);
}

/*
//!  called @ proxy client / server from other point of view to enable forwarding of multicast UDP streams over a TLS over Z over TCP connection, defines the port in case it is not already connected due to another stream
//! to identify multiple streams there's a small header before each packet encoded in the TCP stream, defining multicast address and multicast port
inline void setMCStreamForward(IRPC* rpc, const std::string& service, const std::string streamName, uint16_t proxyPort, IRemoteProcedureCaller* caller, uint32_t id = SETMCSTREAMFORWARD){
	rpc->callRemoteProcedure("setMCStreamForward", std::vector<IRPCValue*>{createRPCValue(service), createRPCValue(streamName), createRPCValue(proxyPort)}, caller, id);
}
*/

}

#endif
