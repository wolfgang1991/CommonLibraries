#ifndef STREAM_PROXY_H_
#define STREAM_PROXY_H_

class StreamProxy;

//! SSL is used
class StreamProxy{

	StreamProxyPrivate* p;
	
	public:
	
	//! controlPort: listens for incoming control connections
	//! dataPort: listens for incoming connections for data transfer
	//! pingTimeout: in milliseconds
	StreamProxy(uint16_t controlPort, uint16_t dataPort, uint32_t pingTimeout);
	
	~StreamProxy();
	
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
	
	void update();
	
};

#endif
