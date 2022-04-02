#ifndef JSONRPC2Client_H_INCLUDED
#define JSONRPC2Client_H_INCLUDED

#include <IRPC.h>

#include <SimpleSockets.h>
#include <Threading.h>

#include <JSONParser.h>

#include <limits>
#include <map>

#define PING_DISABLE_SEND_PERIOD ~(uint32_t)0

//#define DONT_CHECK_JSON_RPC_VERSION

std::string convertRPCValueToJSONResult(const IRPCValue& value, uint32_t jsonId);

//! values may be empty, jsonId may be NULL if no result is expected
std::string makeJSONRPCRequest(const std::string& procedure, const std::vector<IRPCValue*>& values, uint32_t* jsonId);

//! deletes all Elements from a container e.g. std::vector or std::list etc..
template<typename TContainer>
void deleteAllElements(TContainer& ctr){
	for(auto it = ctr.begin(); it != ctr.end(); ++it){
		delete *it;
	}
}

//! returns a field if available or NULL if not available and removes the field from the object
IRPCValue* stealObjectField(ObjectValue* o, const std::string& key);

//! returns a field with the given type and name (key) if available otherwise NULL
IRPCValue* getObjectField(ObjectValue* o, const std::string& key, IRPCValue::Type type = IRPCValue::UNKNOWN);

//! returns a field with the given type and name (key) if available otherwise NULL
template<typename TRPCValue>
TRPCValue* getObjectField(ObjectValue* o, const std::string& key){
	return (TRPCValue*)getObjectField(o, key, TRPCValue::typeId);
}

//! Implementation for JSON-RPC (only Integers allowed and handled for the ids in JSON-RPC)
class JSONRPC2Client : public IRPCClient{

	private:
	
	//for main thread:
	ClientState syncedState;
	double syncedLastReceived;//time in s
	std::list<std::string> mainToSend;
	std::list<IRPCValue*> mainToReceive;
	//for receiving results:
	std::map<uint32_t, std::pair<IRemoteProcedureCaller*, uint32_t> > jsonId2Caller;//jsonId -> (Caller, idFromCaller)
	uint32_t maxJsonId;
	std::list<uint32_t> reusableIds;//old Ids which can be reused
	//for receiving calls:
	std::map<std::string, IRemoteProcedureCallReceiver*> receivers;
	
	//for synchronization:
	Mutex mutexSync;
	bool areTherePendingSends;
	double lastReceived;//time in s
	ClientState state;
	bool syncExit;
	bool mustJoin;
	std::list<std::string> syncToSend;
	std::list<IRPCValue*> syncToReceive;
	
	//for clientMain thread:
	IMetaProtocolHandler* metaProtocolHandler;
	std::list<std::string> clientToSend;
	std::list<IRPCValue*> clientToReceive;
	JSONParser* parser;
	IIPAddress* address;
	ICommunicationEndpoint* socket;
	uint32_t pingSendPeriod, pingTimeout, connectTimeout;
	
	static void* clientMain(void* p);
	
	Thread clientThread;
	
	//! for main thread
	void handleEntity(IRPCValue* entity);
	
	public:
	
	JSONRPC2Client();
	
	~JSONRPC2Client();
	
	bool callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller = NULL, uint32_t id = 0, bool deleteValues = true);
	
	void registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver);
	
	void unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver = NULL);

	void connect(const IIPAddress& address, uint32_t pingSendPeriod, uint32_t pingTimeout, uint32_t connectTimeout, IMetaProtocolHandler* metaProtocolHandler = NULL);
	
	//! Alternative to connect, also works with arbitrary communication endpoints (only need to implement send and recv)
	void useSocket(ICommunicationEndpoint* socket, uint32_t pingTimeout, uint32_t pingSendPeriod = PING_DISABLE_SEND_PERIOD);
	
	void update();
	
	IRPCClient::ClientState getState() const;
	
	bool isConnected() const;
	
	void disconnect();
	
	void removeProcedureCaller(IRemoteProcedureCaller* caller);
	
	double getLastReceiveTime();
	
	void flush();
	
};

#endif
