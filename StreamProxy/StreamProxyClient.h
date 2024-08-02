#ifndef StreamProxyClient_H_
#define StreamProxyClient_H_

#include "StreamProxyAPI.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class X509Cert;
class IIPAddress;
struct StreamProxyClientPrivate;

class StreamProxyClient{

	StreamProxyClientPrivate* p;

	public:
	
	StreamProxyClient(uint32_t connectTimeout = 2000, uint32_t pingTimeout = StreamProxyAPI::proxyPingTimeout);
	
	~StreamProxyClient();
	
	//! blocking connect to proxy, calls OnConnectResult with success if done, addressList will be deleted
	bool connect(const std::list<IIPAddress*>& addressList, std::shared_ptr<X509Cert> publicCert);
	
	void disconnect();
	
	bool isConnected() const;
	
	//! log in @ proxy
	void login(const std::string& password, const std::function<void(bool)>& OnLoginResult);
	
	//! register a service, login @proxy required before, callback OnNewTCPClient is called in case of a new client
	void registerService(const std::string& service, const std::string& password, const std::function<void(bool)>& OnRegisterServiceResult, const std::function<void(const std::string&, ASocket*)>& OnNewTCPClient);
	
	//! authenticate with a remote service
	void authenticate(const std::string& service, const std::string& password, const std::function<void(const std::string&, bool)>& OnAuthenticateResult);
	
	//! blocking to create a tcp connection with given service, prior authentication required
	//! socket==NULL if unsuccessful, otherwise new connected TCP socket
	ASocket* connectTCP(const std::string& service);
	
	//! retrieves service list available @ proxy, login required
	void getServiceList(const std::function<void(const std::vector<std::string>&)>& OnServiceListResult);
	
	//! updates all the rpc stuff, required that the callbacks are called
	void update();

};

#endif
