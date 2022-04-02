#ifndef SSLSocket_H_INCLUDED
#define SSLSocket_H_INCLUDED

#ifndef NO_OPENSSL

#include "SimpleSockets.h"

#include <vector>
#include <cstdint>
#include <string>

class SSLSocketPrivate;
class SSLContextPrivate;

class SSLContext{
	friend class SSLContextPrivate;
	friend class SSLSocketPrivate;
	
	SSLContextPrivate* p;
	
	public:
	
	enum Mode{
		CLIENT,
		SERVER,
		MODE_COUNT
	};
	
	SSLContext(Mode mode);
	
	virtual ~SSLContext();
	
	//! Content of data is the same as in case of a key file created by openssl command line tool
	//! Hint: You can use loadFileWithAssetSupportIntoVector from IrrlichtExtensions/utilities.h
	//! returns true if successful
	bool usePrivateKey(const std::vector<char>& data);
	
	//! Content of data is the same as in case of a cert file created by openssl command line tool
	//! Hint: You can use loadFileWithAssetSupportIntoVector from IrrlichtExtensions/utilities.h
	//! returns true if successful
	bool useCertificate(const std::vector<char>& data);
	
	//! @sa usePrivateKey
	bool usePrivateKeyFromFile(const std::string& path);
	
	//! @sa useCertificate
	bool useCertificateFromFile(const std::string& path);
	
};

class SSLSocket : public ICommunicationEndpoint{
	friend class SSLSocketPrivate;
	
	SSLSocketPrivate* p;
	
	public:
	
	//! mustDeleteSlaveSocket: true if slaveSocket shall be deleted upon destruction
	//! slaveSocket: should be a reliable stream oriented socket (e.g. TCP) or any other reliable stream oriented communication endpoint
	SSLSocket(SSLContext* c, ICommunicationEndpoint* slaveSocket, bool mustDeleteSlaveSocket = true);//, CertStore* store
	
	virtual ~SSLSocket();
	
	int32_t recv(char* buf, uint32_t bufSize);
	
	bool send(const char* buf, uint32_t bufSize);
	
	//! called by the SSL Server, true if ssl connection has been established
	bool accept();
	
	//! called by the SSL Client to establish a SSL connection / negotation, returns true if successful
	bool connect();
	
};

#endif

#endif
