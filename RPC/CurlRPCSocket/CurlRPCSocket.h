#ifndef CurlRPCSocket_H_INCLUDED
#define CurlRPCSocket_H_INCLUDED

#include <SimpleSockets.h>

#include <functional>

class CurlRPCSocketPrivate;

std::function<bool(char)> createJSONRPCFullRequestDetectionFunction();

//! ISocket Implementation for Curl intended to use with rpc calls, a parser function is used to determine if the requests are complete and ready to send
//! IMPORTANT: asserts that curl_global_init has been called before
class CurlRPCSocket : public ISocket{
	friend class CurlRPCSocketPrivate;

	private:
	
	CurlRPCSocketPrivate* prv;

	public:
	
	enum Method{
		HTTP_POST,
		METHOD_COUNT
	};
	
	//! maxPendingBytes: maximum amount of bytes buffered without sending, send will fail if this number is exceeded
	//! isFullRequestDetected: function which returns true if full requests are detected (e.g. balanced brackets and " in case of JSON-RPC)
	//! verifySSLHost: if true: certificate must indicate that the server is the server to which you meant to connect, or the connection fails. Simply put, it means it has to have the same name in the certificate as is in the URL you operate against.
	//! verifySSLPeer: if true: Curl verifies whether the certificate is authentic, i.e. that you can trust that the server is who the certificate says it is. This trust is based on a chain of digital signatures, rooted in certification authority (CA) certificates you supply. curl uses a default bundle of CA certificates (the path for that is determined at build time) and you can specify alternate certificates with the CURLOPT_CAINFO option or the CURLOPT_CAPATH option. 
	CurlRPCSocket(const std::string& url, bool verifySSLHost, bool verifySSLPeer, const std::function<bool(char)>& isFullRequestDetected = createJSONRPCFullRequestDetectionFunction(), uint32_t maxPendingBytes = 10485760, Method method = HTTP_POST);
	
	~CurlRPCSocket();
	
	uint32_t getAvailableBytes() const;
	
	uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false);
	
	bool send(const char* buf, uint32_t bufSize);

};

#endif
