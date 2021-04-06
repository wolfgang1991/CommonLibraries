#ifndef RequestBasedJSONRPC2Client_H_INCLUDED
#define RequestBasedJSONRPC2Client_H_INCLUDED

#include <IRPC.h>

#include <functional>
#include <sstream>

//! Interface for sending requests / receiving responses via a suitable protocol
class IRequestSender{
	
	public:
	
	enum ResponseType{
		SUCCESS,
		ERROR_NO_CONNECTION,
		ERROR_BAD_RESPONSE,
		RESPONSE_TYPE_COUNT
	};
	
	virtual ~IRequestSender(){}
	
	//! executes a blocking request and returns true if successful, the second parameter holds an error message if not successful or the result if successful
	//! may be called in a separate thread
	virtual ResponseType sendRequest(const std::string& toSend, std::string& result) = 0;
	
};

class RequestBasedJSONRPC2ClientPrivate;

//! A JSON-RPC2 Client over a request-response based protocol. Only the client can call methods, the server just replies.
//! Deleting this may take a long time since pending requests need to be completed for safety of the underlying protocol dependent allocations
class RequestBasedJSONRPC2Client : public IRPC{
	friend class RequestBasedJSONRPC2ClientPrivate;
	
	private:
	
	RequestBasedJSONRPC2ClientPrivate* p;
	
	public:
	
	//! autoRetries: in case of a unsuccessful request: how many times shall it be retried automatically before calling IRemoteProcedureCaller::OnProcedureError
	RequestBasedJSONRPC2Client(IRequestSender* sender, uint32_t autoRetries);
	
	~RequestBasedJSONRPC2Client();
	
	bool callRemoteProcedure(const std::string& procedure, const std::vector<IRPCValue*>& values, IRemoteProcedureCaller* caller = NULL, uint32_t id = 0, bool deleteValues = true);
	
	void registerCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver);
	
	void unregisterCallReceiver(const std::string& procedure, IRemoteProcedureCallReceiver* receiver = NULL);
	
	void removeProcedureCaller(IRemoteProcedureCaller* caller);
	
	void update();
	
};

#ifndef NO_CURL

//! A request sender implementation for CURL
class CURLRequestSender : public IRequestSender{

	private:
	
	void* curl;
	std::stringstream response;
	bool isGoodResponse;
	
	void* hs;
	
	static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
	
	static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);

	public:
	
	CURLRequestSender(const std::string& url, bool skipPeerVerification, bool skipHostnameVerification);
	
	~CURLRequestSender();
	
	ResponseType sendRequest(const std::string& toSend, std::string& result);
	
};

#endif

#endif
