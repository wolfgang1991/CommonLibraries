#ifndef SSLSocket_H_INCLUDED
#define SSLSocket_H_INCLUDED

#ifndef NO_OPENSSL

#include "SimpleSockets.h"

#include <vector>
#include <cstdint>
#include <string>

/*class CertStore;*/
class SSLSocketPrivate;

class SSLSocket : public ISocket{
	friend class SSLSocketPrivate;
	
	SSLSocketPrivate* p;
	
	public:
	
	enum Mode{
		CLIENT,
		SERVER,
		MODE_COUNT
	};
	
	//! mustDeleteSlaveSocket: true if slaveSocket shall be deleted upon destruction
	//! slaveSocket: should be a reliable stream oriented socket (e.g. TCP)
	SSLSocket(ISocket* slaveSocket, Mode mode, bool mustDeleteSlaveSocket = true);//, CertStore* store
	
	virtual ~SSLSocket();
	
	uint32_t getAvailableBytes() const;
	
	uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false);
	
	bool send(const char* buf, uint32_t bufSize);
	
/*	void setCertStore(CertStore* store);*/
/*	*/
/*	CertStore* getCertStore() const;*/
	
	//! called by the SSL Server, true if ssl connection has been established
	bool accept();
	
	//! called by the SSL Client to establish a SSL connection / negotation, returns true if successful
	bool connect();
	
	//! true if successful
	bool addPrivateKey(const char* buffer, uint32_t bufferSize);
	
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

/*//! Holds certificates for a SSLSocket e.g. CA Certs. DO NOT delete if still used by a socket*/
/*class CertStore{*/
/*	friend class SSLSocket;*/

/*	private:*/
/*	*/
/*	void* store;*/
/*	*/
/*	public:*/
/*	*/
/*	CertStore();*/
/*	*/
/*	virtual ~CertStore();*/
/*	*/
/*	//! loadFunction: function to load the cert (e.g. file content) into a new buffer with a uint32_t output length*/
/*	//! returns true if successful*/
/*	bool loadX509PEMCert(const std::function<bool(char*&, uint32_t&)>& loadFunction);*/
/*	*/
/*	//! loads it from a file*/
/*	bool loadX509PEMCert(const std::string& path);*/
/*	*/
/*	//! loads it from an existing buffer*/
/*	bool loadX509PEMCert(char* buf, uint32_t bufSize);*/
/*	*/
/*};*/

#endif

#endif
