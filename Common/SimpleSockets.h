#ifndef SIMPLESOCKETS_H_INCLUDED
#define SIMPLESOCKETS_H_INCLUDED

#include <ICommunicationEndpoint.h>

#include <string>
#include <list>
#include <cstdint>

#define SIMPLESOCKETS_WIN (defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64))

#if SIMPLESOCKETS_WIN
#include <ws2tcpip.h>
#undef ERROR
#undef small
#undef ABSOLUTE
#undef TRANSPARENT
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#endif

class IIPAddress{

	public:
	
	enum IP_VERSION{IPV4, IPV6, IP_COUNT};
	
	virtual IP_VERSION getIPVersion() const = 0;
	
	virtual std::string getAddressAsString() const = 0;
	
	virtual uint16_t getPort() const = 0;
	
	virtual void setPort(uint16_t port) = 0;
	
	virtual IIPAddress* createNewCopy() const = 0;
	
	virtual ~IIPAddress(){}

};

class IPv4Address : public IIPAddress{
	
	private:
	
	sockaddr_in addr;
	
	public:
	
	IPv4Address(const std::string& addressString, uint16_t port);
	
	IP_VERSION getIPVersion() const;
	
	std::string getAddressAsString() const;
	
	uint16_t getPort() const;
	
	void setPort(uint16_t port);
	
	sockaddr_in& getInternalRepresentation();
	
	const sockaddr_in& getInternalRepresentation() const;
	
	void setInternalRepresentation(const sockaddr_in& r);
	
	IPv4Address* createNewCopy() const;
	
	bool operator<(const IPv4Address& other) const;

};

class IPv6Address : public IIPAddress{
	
	private:
	
	sockaddr_in6 addr;
	
	public:
	
	IPv6Address(const std::string& addressString, uint16_t port);
	
	IP_VERSION getIPVersion() const;
	
	std::string getAddressAsString() const;
	
	uint16_t getPort() const;
	
	void setPort(uint16_t port);
	
	sockaddr_in6& getInternalRepresentation();
	
	const sockaddr_in6& getInternalRepresentation() const;
	
	void setInternalRepresentation(const sockaddr_in6& r);
	
	IPv6Address* createNewCopy() const;
	
	bool operator<(const IPv6Address& other) const;

};

//! general interface for sockets
class ISocket : public ICommunicationEndpoint{

	public:
	
	//! returns the amount of readable bytes
	virtual uint32_t getAvailableBytes() const = 0;
	
	virtual uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking) = 0;

	int32_t recv(char* buf, uint32_t bufSize){
		return recv(buf, bufSize, false);
	}
	
	//! true if buf has been sent (does not gurantee reception on other side)
	virtual bool send(const char* buf, uint32_t bufSize) = 0;
	
	virtual ~ISocket(){}

};

//! general implementation for "true" sockets like tcp and udp sockets
class ASocket : public ISocket{

	protected:
	
	int socketHandle;
	
	bool shallTryRestore;
	
	int32_t restoreReceiveSize;
	
	ASocket();
	
	#if SIMPLESOCKETS_WIN
	int isBlocking;//! 0 false, 1 true, 2 undefined
	void handleBlocking(bool shallBeBlocking);
	#endif

	public:
	
	//! returns the amount of readable bytes
	virtual uint32_t getAvailableBytes() const;
	
	virtual uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false) override;
	
	//! true if buf has been sent (does not gurantee reception on other side)
	virtual bool send(const char* buf, uint32_t bufSize);
	
	//! reusePort: if true multiple sockets can bind to this port (UDP: for send/receive TCP: for listen)
	virtual bool bind(int port, bool reusePort = false) = 0;
	
	//! recreates the socket and restores the state/connection
	virtual bool restore();
	
	//! try restore and remember if unsucessful, if unsuccessful don't try again
	bool tryRestoreOnce();
	
	virtual int getSocketHandle() const;
	
	virtual bool setReceiveBufferSize(uint32_t size);
	
	virtual ~ASocket();

};

class IPv4Socket : public ASocket{
	
	protected:
	
	int restoreBind;
	bool restoreReusePort;
	
	IPv4Socket();
	
	public:
	
	virtual bool bind(int port, bool reusePort = false);
	
	virtual bool restore();
	
	virtual ~IPv4Socket(){}

};

class IPv6Socket : public ASocket{
	
	protected:
	
	int restoreBind;
	bool restoreReusePort;
	bool restoreIPv4ReceptionEnabled;

	IPv6Socket();
	
	void setIPv4ReceptionEnabled(bool enabled);
	
	public:
	
	virtual bool bind(int port, bool reusePort = false);
	
	virtual bool restore();
	
	virtual ~IPv6Socket(){}

};

class IPv4UDPSocket : public IPv4Socket{

	private:
	
	bool boundOrSent;
	
	void init(int socketHandle);
	
	IPv4Address targetAddress;
	IPv4Address lastReceivedAddress;

	public:
	
	//! create new socket
	IPv4UDPSocket();
	
	//! use existing "native" socket
	IPv4UDPSocket(int socketHandle);
	
	bool bind(int port, bool reusePort = false);
	
	bool restore();
	
	void setUDPTarget(const IPv4Address& targetAddress);
	
	const IPv4Address& getUDPTarget() const; 
	
	//! true if successful
	bool send(const char* buf, uint32_t bufSize);
	
	uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false);
	
	const IPv4Address& getLastDatagramAddress() const;

};

class IPv6UDPSocket : public IPv6Socket{

	private:
	
	bool boundOrSent;
	
	void init(int socketHandle);
	
	IPv6Address targetAddress;
	IPv6Address lastReceivedAddress;

	public:
	
	//! create new socket
	IPv6UDPSocket();
	
	//! use existing "native" socket
	IPv6UDPSocket(int socketHandle);
	
	bool bind(int port, bool reusePort = false);
	
	//! joins the "ff02::1" link local multicast group at the selected interface
	//! WARNING1: May hang on different plattforms (like iOS) if interface is down
	//! WARNING2: This state is not restored when restore is called (breaks workaround for iOS)
	void joinLinkLocalMulticastGroup(uint32_t multicastInterfaceIndex);
	
	bool restore();
	
	void setUDPTarget(const IPv6Address& targetAddress);
	
	const IPv6Address& getUDPTarget() const; 
	
	//! true if successful
	bool send(const char* buf, uint32_t bufSize);
	
	uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false);
	
	const IPv6Address& getLastDatagramAddress() const;

};

class IPv4TCPSocket : public IPv4Socket{
	
	protected:
	
	int32_t restoreTimeout;
	IPv4Address restoreAddress;
	
	int restoreListen;

	public:
	
	//! create new socket
	IPv4TCPSocket();
	
	//! use existing "native" socket
	IPv4TCPSocket(int socketHandle);
	
	bool restore();
	
	bool listen(int maxPendingConnections);
	
	//! timeout in ms (if NOT equal 0 it blocks until there's something to accept or timeout)
	//! peerAddress: if not NULL it will be filled with the peer address
	IPv4TCPSocket* accept(uint32_t timeout = 0, IPv4Address* peerAddress = NULL);
	
	//! timeout in ms
	bool connect(const IPv4Address& address, uint32_t timeout);

};

class IPv6TCPSocket : public IPv6Socket{
	
	protected:
	
	int32_t restoreTimeout;
	IPv6Address restoreAddress;
	
	int restoreListen;
	
	public:
	
	//! create new socket
	IPv6TCPSocket();
	
	//! use existing "native" socket
	IPv6TCPSocket(int socketHandle);
	
	bool restore();
	
	bool listen(int maxPendingConnections);
	
	//! timeout in ms (if not equal 0 it blocks until there's something to accept or timeout)
	IPv6TCPSocket* accept(uint32_t timeout = 0, IPv6Address* peerAddress = NULL);
	
	//! timeout in ms
	bool connect(const IPv6Address& address, uint32_t timeout);

};

//! returns NULL if unsuccessful, timeout in ms (total timeout = timeout * addressList.size())
ASocket* connectSocketForAddressList(const std::list<IIPAddress*>& addressList, uint32_t timeout);

//! WARNING: NO TIMEOUT; portToFill: port which is filled into the results
std::list<IIPAddress*> queryIPAddressesForHostName(std::string hostName, uint16_t portToFill = 0);

//! if tcp create tcp socket and connect with tcpConnectTimeout, else create udp and set target
//! returns NULL if no known ip address or tcp connection failed
ASocket* createSocketForHostName(bool tcp, std::string hostName, uint16_t port, uint32_t tcpConnectTimeout);

template<typename TUDPSocket, typename TContainer>
bool sendUDPToAddresses(TUDPSocket& s, const TContainer& addressList, const char* buf, uint32_t bufSize){
	bool succ = true;
	for(auto it=addressList.begin(); it!=addressList.end(); ++it){
		s.setUDPTarget(*it);
		bool res = s.send(buf, bufSize);
		succ = succ && res;
	}
	return succ;
}

//! returns the ipv4 broadcast addresses for all interfaces
std::list<IPv4Address> queryIPv4BroadcastAdresses(uint16_t portToFill = 0);

#endif
