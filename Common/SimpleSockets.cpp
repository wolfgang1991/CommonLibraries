#include "SimpleSockets.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

#if SIMPLESOCKETS_WIN
//TODO
#else
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#if defined(__ANDROID__)// || defined(__linux__)
#include <android/log.h>
#define USE_ANDROID_BRD_ADDR_WORKAROUND
#endif

#if !defined(USE_ANDROID_BRD_ADDR_WORKAROUND) && !SIMPLESOCKETS_WIN
#include <ifaddrs.h>
#endif

#if defined(__APPLE__) || defined(MACOSX)
#define MSG_NOSIGNAL SO_NOSIGPIPE
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP // future use
#endif

#if SIMPLESOCKETS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

//Exceptions and signals are dangerous because depending on the OS dependent implementation of the POSIX sockets there can be undocumented errors.
//In some cases it still can be a good idea to simply continue, therefore a crash is not desired.
//For this reason exceptions and signals should be disabled before release. Nevertheless the errors will appear in the log (see handleErrorMessage).
//#define USE_EXCEPTIONS
//#define USE_SIGNALS

#ifdef USE_SIGNALS
#include <csignal>
#endif

#define HAS_EXECINFO defined(__linux__) && defined(__GNUC__) && !defined(__APPLE__) && !defined(__ANDROID__)

#if HAS_EXECINFO
#include <execinfo.h>
#endif

//To simulate a bad connections a delay can be introduced for each connect, send, receive and close
//#define SIMULARE_BAD_CONNECTION
#ifndef CONNECT_DELAY
#define CONNECT_DELAY 500
#endif
#ifndef SEND_DELAY
#define SEND_DELAY 10//max rate for tcp is BAD_CONN_SIM_LIMIT_SIMULTANEOUS_SEND*1000/SEND_DELAY
#endif 
#ifndef RECV_DELAY
#define RECV_DELAY 10//max rate for tcp is BAD_CONN_SIM_LIMIT_RECEIVE_BUFFER*1000/RECV_DELAY
#endif
#ifndef CLOSE_DELAY
#define CLOSE_DELAY 500
#endif
//Macros for delays
#ifdef SIMULARE_BAD_CONNECTION
#include "timing.h"
#define ONCONNECT delay(CONNECT_DELAY);
#define ONSEND delay(SEND_DELAY);
#define ONRECEIVE(BLOCKING) if(BLOCKING){delay(RECV_DELAY);}
#define ONCLOSE delay(CLOSE_DELAY);
#define BAD_CONN_SIM_LIMIT_RECEIVE_BUFFER 5 // useful to simulate "fragmentation" and slow receiving (used in ASocket::recv)
#define BAD_CONN_SIM_LIMIT_SIMULTANEOUS_SEND 5 // useful to simulate "fragmentation" and slow sending (used in ASocket::send)
#else
#define ONCONNECT ;
#define ONSEND ;
#define ONRECEIVE(BLOCKING) ;
#define ONCLOSE ;
#endif

static volatile bool initialized = false;
#if SIMPLESOCKETS_WIN
static WSADATA wsa;
#endif

static void checkAndInitGlobally(){
	if(!initialized){
		initialized = true;
		#if SIMPLESOCKETS_WIN
		int res = WSAStartup(MAKEWORD(2,0),&wsa);
		assert(res==0);
		#endif
	}
}

#if SIMPLESOCKETS_WIN
static int inet_pton(int af, const char* src, void* dst){
	sockaddr_storage addr;
	char srcStr[INET6_ADDRSTRLEN];
	memset(&addr, 0, sizeof(addr));
	strncpy(srcStr, src, INET6_ADDRSTRLEN);
	srcStr[INET6_ADDRSTRLEN-1] = 0;
	int size = sizeof(addr);
	if(WSAStringToAddress(srcStr, af, NULL, (sockaddr*)&addr, &size)==0){
		if(af==AF_INET){
			*(in_addr*)dst = ((sockaddr_in*)&addr)->sin_addr;
			return 1;
		}else if(af==AF_INET6){
			*(in6_addr*)dst = ((sockaddr_in6*)&addr)->sin6_addr;
			return 1;
		}
	}
	return 0;
}

static const char* inet_ntop(int af, const void* src, char* dst, socklen_t size){
	sockaddr_storage addr;
	memset(&addr, 0, sizeof(addr));
	addr.ss_family = af;
	if(af==AF_INET){
		((sockaddr_in*)&addr)->sin_addr = *(in_addr*)src;
	}else if(af==AF_INET6){
		((sockaddr_in6*)&addr)->sin6_addr = *(in6_addr*)src;
	}else{
		return NULL;
	}
	unsigned long s = size;
	return (WSAAddressToString((sockaddr*)&addr, sizeof(addr), NULL, dst, &s)==0)?dst:NULL;
}
#endif

static void handleErrorMessage(const char* error = NULL){
	#if SIMPLESOCKETS_WIN
	const char* finalError = error;
	if(error==NULL){
		static char errorstr[512];
		errorstr[0] = '\0';
		int errorCode = WSAGetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,errorCode, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),errorstr,sizeof(errorstr),NULL);
		finalError = errorstr;
		std::cout << "WSAGetLastError: " << errorCode << std::endl;
	}
	#else
	const char* finalError = error==NULL?strerror(errno):error;
	std::cout << "errno: " << errno << std::endl;
	#endif
	std::cout << "socket error: " << finalError << std::endl;
	#if HAS_EXECINFO
	size_t size = 100;
	void* array[100];
	size = backtrace(array, size);
	backtrace_symbols_fd(array, size, STDOUT_FILENO);
	#endif
	#if defined(__ANDROID__)
	__android_log_print(ANDROID_LOG_ERROR, "SimpleSockets", "socket error: %s\n", finalError);
	#endif
	#ifdef USE_EXCEPTIONS
	throw finalError;
	#elif defined(USE_SIGNALS)
	std::raise(SIGINT);
	#endif
}

IPv4Address::IPv4Address(const std::string& addressString, uint16_t port){
	checkAndInitGlobally();
	memset(&addr, 0, sizeof(addr));
	if(inet_pton(AF_INET, addressString.c_str(), &(addr.sin_addr))!=1){
		handleErrorMessage("Invalid IP address");
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
}

bool IPv4Address::operator<(const IPv4Address& other) const{
	if(addr.sin_addr.s_addr==other.addr.sin_addr.s_addr){
		return addr.sin_port<other.addr.sin_port;
	}
	return addr.sin_addr.s_addr<other.addr.sin_addr.s_addr;
}

IPv4Address* IPv4Address::createNewCopy() const{
	return new IPv4Address(*this);
}

IIPAddress::IP_VERSION IPv4Address::getIPVersion() const{
	return IIPAddress::IPV4;
}

std::string IPv4Address::getAddressAsString() const{
	char buf[INET_ADDRSTRLEN];
	if(inet_ntop(AF_INET, &(addr.sin_addr), buf, INET_ADDRSTRLEN)==NULL){
		handleErrorMessage();
		return "0.0.0.0";
	}
	return std::string(buf);	
}

uint16_t IPv4Address::getPort() const{
	return ntohs(addr.sin_port);
}

void IPv4Address::setPort(uint16_t port){
	addr.sin_port = htons(port);
}

sockaddr_in& IPv4Address::getInternalRepresentation(){
	return addr;
}

const sockaddr_in& IPv4Address::getInternalRepresentation() const{
	return addr;
}

void IPv4Address::setInternalRepresentation(const sockaddr_in& r){
	addr = r;
}

IPv6Address::IPv6Address(const std::string& addressString, uint16_t port){
	checkAndInitGlobally();
	memset(&addr, 0, sizeof(addr));
	if(inet_pton(AF_INET6, addressString.c_str(), &(addr.sin6_addr))!=1){
		handleErrorMessage("Invalid IP address");
	}
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
}

bool IPv6Address::operator<(const IPv6Address& other) const{
	const uint8_t* this_s6_addr = addr.sin6_addr.s6_addr;
	const uint8_t* other_s6_addr = other.addr.sin6_addr.s6_addr;
	for(int i=0; i<16; i++){
		if(this_s6_addr[i]<other_s6_addr[i]){
			return true;
		}else if(this_s6_addr[i]>other_s6_addr[i]){
			return false;
		}
	}
	return addr.sin6_port<other.addr.sin6_port;
}

IPv6Address* IPv6Address::createNewCopy() const{
	return new IPv6Address(*this);
}

IIPAddress::IP_VERSION IPv6Address::getIPVersion() const{
	return IIPAddress::IPV6;
}

std::string IPv6Address::getAddressAsString() const{
	char buf[INET6_ADDRSTRLEN];
	if(inet_ntop(AF_INET6, &(addr.sin6_addr), buf, INET6_ADDRSTRLEN)==NULL){
		handleErrorMessage();
		return "::";
	}
	return std::string(buf);
}

uint16_t IPv6Address::getPort() const{
	return ntohs(addr.sin6_port);
}

void IPv6Address::setPort(uint16_t port){
	addr.sin6_port = htons(port);
}

sockaddr_in6& IPv6Address::getInternalRepresentation(){
	return addr;
}

const sockaddr_in6& IPv6Address::getInternalRepresentation() const{
	return addr;
}

void IPv6Address::setInternalRepresentation(const sockaddr_in6& r){
	addr = r;
}

int ASocket::getSocketHandle() const{
	return socketHandle;
}

bool ASocket::setReceiveBufferSize(uint32_t size){
	#ifdef SIMPLESOCKETS_WIN
	int s = size;
	if(setsockopt(socketHandle, SOL_SOCKET, SO_RCVBUF, (const char*)&s, sizeof(size))!=-1){
	#else
	if(setsockopt(socketHandle, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size))!=-1){
	#endif
		restoreReceiveSize = size;
		return true;
	}
	return false;
}

bool ASocket::restore(){
	if(restoreReceiveSize>=0){
		return setReceiveBufferSize(restoreReceiveSize);
	}
	return true;
}

ASocket::ASocket(){
	checkAndInitGlobally();
	restoreReceiveSize = -1;
	socketHandle = -1;
	shallTryRestore = true;
	#if SIMPLESOCKETS_WIN
	isBlocking = 2;
	#endif
}

#if SIMPLESOCKETS_WIN
void ASocket::handleBlocking(bool shallBeBlocking){
	if(socketHandle!=-1 && isBlocking!=(int)shallBeBlocking){
		isBlocking = (int)shallBeBlocking;
		unsigned long mode = shallBeBlocking?0:1;
		if(ioctlsocket(socketHandle, FIONBIO, &mode)!=0){
			handleErrorMessage();
		}
	}
}
#endif

bool ASocket::tryRestoreOnce(){
	if(shallTryRestore){
		shallTryRestore = restore();
		return shallTryRestore;
	}
	return false;
}

ASocket::~ASocket(){
	if(socketHandle!=-1){
		ONCLOSE
		#if SIMPLESOCKETS_WIN
		int res = closesocket(socketHandle);
		if(res<0){
			handleErrorMessage();
		}
		#else
		int res = ::close(socketHandle);
		if(res<0 && errno!=EIO && errno!=EINTR){
			handleErrorMessage();
		}
		#endif
	}
}

uint32_t ASocket::getAvailableBytes() const{
	#if SIMPLESOCKETS_WIN
	u_long res = 0;
	if(ioctlsocket(socketHandle, FIONREAD, &res)!=0){
		handleErrorMessage();
	#else
	int res = 0;
	if(ioctl(socketHandle, FIONREAD, &res)!=0){
		handleErrorMessage();
	#endif
		return 0;
	}
	return (uint32_t)res;
}
	
uint32_t ASocket::recv(char* buf, uint32_t bufSize, bool readBlocking){
	ONRECEIVE(readBlocking)
	#ifdef BAD_CONN_SIM_LIMIT_RECEIVE_BUFFER
	bufSize = bufSize>BAD_CONN_SIM_LIMIT_RECEIVE_BUFFER?BAD_CONN_SIM_LIMIT_RECEIVE_BUFFER:bufSize;
	#endif
	#if SIMPLESOCKETS_WIN
	handleBlocking(readBlocking);
	int read = ::recv(socketHandle, buf, bufSize, 0);
	if(read<0){
		int errCode = WSAGetLastError();
		if(errCode!=WSAEWOULDBLOCK && errCode!=WSAEINPROGRESS && errCode!=WSAEALREADY && errCode!=WSAECONNRESET && errCode!=WSAEHOSTUNREACH){
			handleErrorMessage();
		}
		return 0;
	}
	#else
	ssize_t read = ::recv(socketHandle, buf, bufSize, readBlocking?0:MSG_DONTWAIT);
	if(read<0){
		if(errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=ECONNRESET && errno!=EHOSTUNREACH){
			if(errno==ENOTCONN){
				if(tryRestoreOnce()){
					ONRECEIVE(readBlocking)
					read = ::recv(socketHandle, buf, bufSize, readBlocking?0:MSG_DONTWAIT);
					if(read>=0){
						return read;
					}else if(errno==EAGAIN || errno==EWOULDBLOCK || errno==ECONNRESET || errno==EHOSTUNREACH){
						return 0;
					}
				}
			}
			handleErrorMessage();
			return 0;
		}else{
			return 0;
		}
	}
	#endif
	return read;
}

static inline bool hlp_send(ASocket* socket, const char* buf, uint32_t bufSize){
	ONSEND
	#if SIMPLESOCKETS_WIN
	bool success = ::send(socket->getSocketHandle(), buf, bufSize, 0)!=SOCKET_ERROR;
	int errCode = WSAGetLastError();
	if(!success && errCode!=WSAECONNRESET && errCode!=WSAENOTCONN){
		handleErrorMessage();
	}
	return success;
	#else
	if(::send(socket->getSocketHandle(), buf, bufSize, MSG_NOSIGNAL)!=-1){
		return true;
	}else if(errno==ENOTCONN){
		if(socket->tryRestoreOnce()){
			ONSEND
			return ::send(socket->getSocketHandle(), buf, bufSize, MSG_NOSIGNAL)!=-1;
		}
	}else{
		handleErrorMessage();
	}
	return false;
	#endif
}
	
bool ASocket::send(const char* buf, uint32_t bufSize){
	#ifdef BAD_CONN_SIM_LIMIT_SIMULTANEOUS_SEND
		bool success = true;
		while(success && bufSize>0){
			uint32_t toSend = bufSize>BAD_CONN_SIM_LIMIT_SIMULTANEOUS_SEND?BAD_CONN_SIM_LIMIT_SIMULTANEOUS_SEND:bufSize;
			//std::cout << "toSend: " << toSend << std::endl;
			success = hlp_send(this, buf, toSend);
			bufSize -= toSend;
			buf = &buf[toSend];
		}
		return success;
	#else
	return hlp_send(this, buf, bufSize);
	#endif
}

IPv4Socket::IPv4Socket(){restoreBind = -1; restoreReusePort = false;}

bool IPv4Socket::restore(){
	bool res = true;
	if(restoreBind>=0){
		res = res && bind(restoreBind, restoreReusePort);
	}
	res = res && ASocket::restore();
	return res;
}

bool IPv4Socket::bind(int port, bool reusePort){
	int enable = 1;
	if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable))==-1){//without this it is not possible to bind again shortly after the socket has been closed
		handleErrorMessage();
	}
	if(reusePort){
		#if SIMPLESOCKETS_WIN
		handleErrorMessage("SO_REUSEPORT not supported on windows.");
		#else
		if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable))==-1){
			handleErrorMessage();
		}
		#endif
	}
	restoreReusePort = reusePort;
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if(::bind(socketHandle, (sockaddr*)&addr, sizeof(addr))!=-1){
		restoreBind = port;
		return true;
	}
	return false;
}

static void disableBlockingTimeout(int sockHandle){
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	setsockopt(sockHandle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

IPv4UDPSocket::IPv4UDPSocket():boundOrSent(false),targetAddress("0.0.0.0",0),lastReceivedAddress("0.0.0.0",0){
	init(-1);
}

IPv4UDPSocket::IPv4UDPSocket(int socketHandle):boundOrSent(false),targetAddress("0.0.0.0",0),lastReceivedAddress("0.0.0.0",0){
	init(socketHandle);
}

void IPv4UDPSocket::init(int socketHandle){
	if(socketHandle==-1){
		this->socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}else{
		this->socketHandle = socketHandle;
	}
	int on = 1;
	if(setsockopt(this->socketHandle, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on))==-1){//allow broadcast
		handleErrorMessage();
	}
	disableBlockingTimeout(this->socketHandle);
}

bool IPv4UDPSocket::restore(){
	::close(socketHandle);
	init(-1);
	return IPv4Socket::restore();
}

void IPv4UDPSocket::setUDPTarget(const IPv4Address& targetAddress){
	this->targetAddress = targetAddress;
}

const IPv4Address& IPv4UDPSocket::getUDPTarget() const{
	return targetAddress;
}

bool IPv4UDPSocket::bind(int port, bool reusePort){
	boundOrSent = true;
	return IPv4Socket::bind(port, reusePort);
}

bool IPv4UDPSocket::send(const char* buf, uint32_t bufSize){
	boundOrSent = true;
	ONSEND
	if(sendto(socketHandle, buf, bufSize, 0, (const sockaddr*)(&(targetAddress.getInternalRepresentation())), sizeof(sockaddr_in)) != -1){
		return true;
	}else if(errno==ENOTCONN){//reset by iOS
		if(tryRestoreOnce()){
			ONSEND
			return sendto(socketHandle, buf, bufSize, 0, (const sockaddr*)(&(targetAddress.getInternalRepresentation())), sizeof(sockaddr_in)) != -1;
		}
	}else{
		handleErrorMessage();
	}
	return false;
}

uint32_t IPv4UDPSocket::recv(char* buf, uint32_t bufSize, bool readBlocking){
	if(!boundOrSent){return 0;}
	sockaddr_in addr;
	ONRECEIVE(readBlocking)
	#if SIMPLESOCKETS_WIN
	handleBlocking(readBlocking);
	int len = sizeof(addr);
	int received = ::recvfrom(socketHandle, buf, bufSize, 0, (sockaddr*)(&addr), &len);
	if(received>0){
		lastReceivedAddress.setInternalRepresentation(addr);
		return received;
	}else if(received<0){
		int errCode = WSAGetLastError();
		if(errCode!=WSAEWOULDBLOCK && errCode!=WSAEINPROGRESS && errCode!=WSAEALREADY && errCode!=WSAECONNRESET && errCode!=WSAEHOSTUNREACH){
			handleErrorMessage();
		}
		return 0;
	}
	#else
	socklen_t len = sizeof(addr);
	ssize_t received = recvfrom(socketHandle, buf, bufSize, readBlocking?0:MSG_DONTWAIT, (sockaddr*)(&addr), &len);
	if(received>=0){
		lastReceivedAddress.setInternalRepresentation(addr);
		return received;
	}else if(received<0 && errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EHOSTUNREACH){
		if(errno==ENOTCONN){//reset by iOS
			if(tryRestoreOnce()){
				ONRECEIVE(readBlocking)
				received = recvfrom(socketHandle, buf, bufSize, readBlocking?0:MSG_DONTWAIT, (sockaddr*)(&addr), &len);
				if(received>=0){
					return received;
				}else if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EHOSTUNREACH){
					return 0;
				}
			}
		}
		handleErrorMessage();
		return 0;
	}
	#endif
	return 0;
}
	
const IPv4Address& IPv4UDPSocket::getLastDatagramAddress() const{
	return lastReceivedAddress;
}

IPv6Socket::IPv6Socket(){restoreBind = -1; restoreIPv4ReceptionEnabled = false; restoreReusePort = false;}

bool IPv6Socket::restore(){
	setIPv4ReceptionEnabled(restoreIPv4ReceptionEnabled);
	bool res = true;
	if(restoreBind>=0){
		res = res && bind(restoreBind, restoreReusePort);
	}
	res = res && ASocket::restore();
	return res;
}

void IPv6Socket::setIPv4ReceptionEnabled(bool enabled){
	restoreIPv4ReceptionEnabled = enabled;
	int opt = (int)(!enabled);
	if(setsockopt(socketHandle, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&opt, sizeof(opt))==-1){
		#if SIMPLESOCKETS_WIN
		if(WSAGetLastError()!=WSAEINVAL){
		#else
		if(errno!=EINVAL){
		#endif
			handleErrorMessage();
		}
	}
}

bool IPv6Socket::bind(int port, bool reusePort){
	int enable = 1;
	if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable))==-1){//without this it is not possible to bind again shortly after the socket has been closed
		handleErrorMessage();
	}
	if(reusePort){
		#if SIMPLESOCKETS_WIN
		handleErrorMessage("SO_REUSEPORT not supported on windows.");
		#else
		if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEPORT, (const char*)&enable, sizeof(enable))==-1){
			handleErrorMessage();
		}
		#endif
	}
	restoreReusePort = reusePort;
	sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;
	if(::bind(socketHandle, (sockaddr*)&addr, sizeof(addr))!=-1){
		restoreBind = port;
		return true;
	}
	return false;
}

IPv6UDPSocket::IPv6UDPSocket():boundOrSent(false),targetAddress("::",0),lastReceivedAddress("::",0){
	init(-1);
	setIPv4ReceptionEnabled(true);
}

IPv6UDPSocket::IPv6UDPSocket(int socketHandle):boundOrSent(false),targetAddress("::",0),lastReceivedAddress("::",0){
	init(socketHandle);
	setIPv4ReceptionEnabled(true);
}

bool IPv6UDPSocket::bind(int port, bool reusePort){
	boundOrSent = true;
	return IPv6Socket::bind(port, reusePort);
}

void IPv6UDPSocket::joinLinkLocalMulticastGroup(uint32_t multicastInterfaceIndex){
	ipv6_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));
	inet_pton(AF_INET6, "FF02::1", &(mreq.ipv6mr_multiaddr));
	mreq.ipv6mr_interface = multicastInterfaceIndex;
	if(setsockopt(socketHandle, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*)&mreq, sizeof(mreq))==-1){//allow broadcast == link local multicast
		if(errno!=ENODEV){handleErrorMessage();}
	}
}

void IPv6UDPSocket::init(int socketHandle){
	if(socketHandle==-1){
		this->socketHandle = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	}else{
		this->socketHandle = socketHandle;
	}
	int ttl = 63;
	if(setsockopt(this->socketHandle, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char*)&ttl, sizeof(ttl))==-1){//default ttl may be too low
		handleErrorMessage();
	}
	disableBlockingTimeout(this->socketHandle);
}

bool IPv6UDPSocket::restore(){
	::close(socketHandle);
	init(-1);
	return IPv6Socket::restore();
}
	
void IPv6UDPSocket::setUDPTarget(const IPv6Address& targetAddress){
	this->targetAddress = targetAddress;
}

const IPv6Address& IPv6UDPSocket::getUDPTarget() const{
	return targetAddress;
}

bool IPv6UDPSocket::send(const char* buf, uint32_t bufSize){
	boundOrSent = true;
	ONSEND
	if(sendto(socketHandle, buf, bufSize, 0, (sockaddr*)(&(targetAddress.getInternalRepresentation())), sizeof(sockaddr_in6)) != -1){
		return true;
	}else if(errno==ENOTCONN){//reset by iOS
		if(tryRestoreOnce()){
			ONSEND
			return sendto(socketHandle, buf, bufSize, 0, (sockaddr*)(&(targetAddress.getInternalRepresentation())), sizeof(sockaddr_in6)) != -1;
		}
	}else{
		handleErrorMessage();
	}
	return false;
}

uint32_t IPv6UDPSocket::recv(char* buf, uint32_t bufSize, bool readBlocking){
	if(!boundOrSent){return 0;}
	sockaddr_in6 addr;
	ONRECEIVE(readBlocking)
	#if SIMPLESOCKETS_WIN
	handleBlocking(readBlocking);
	int len = sizeof(addr);
	int received = ::recvfrom(socketHandle, buf, bufSize, 0, (sockaddr*)(&addr), &len);
	if(received>0){
		lastReceivedAddress.setInternalRepresentation(addr);
		return received;
	}else if(received<0){
		int errCode = WSAGetLastError();
		if(errCode!=WSAEWOULDBLOCK && errCode!=WSAEINPROGRESS && errCode!=WSAEALREADY && errCode!=WSAECONNRESET && errCode!=WSAEHOSTUNREACH){
			handleErrorMessage();
		}
		return 0;
	}
	#else
	socklen_t len = sizeof(sockaddr_in6);
	ssize_t received = recvfrom(socketHandle, buf, bufSize, readBlocking?0:MSG_DONTWAIT, (sockaddr*)(&addr), &len);
	if(received>0){
		lastReceivedAddress.setInternalRepresentation(addr);
		return received;
	}else if(received<0 && errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=ECONNRESET && errno!=EHOSTUNREACH){
		if(errno==ENOTCONN){//reset by iOS
			if(tryRestoreOnce()){
				ONRECEIVE(readBlocking)
				received = recvfrom(socketHandle, buf, bufSize, readBlocking?0:MSG_DONTWAIT, (sockaddr*)(&addr), &len);
				if(received>=0){
					return received;
				}else if(errno==EAGAIN || errno==EWOULDBLOCK || errno==ECONNRESET || errno==EHOSTUNREACH){
					return 0;
				}
			}
		}
		handleErrorMessage();
		return 0;
	}
	#endif
	return 0;
}

const IPv6Address& IPv6UDPSocket::getLastDatagramAddress() const{
	return lastReceivedAddress;
}

IPv4TCPSocket::IPv4TCPSocket():restoreTimeout(-1),restoreAddress("0.0.0.0",0),restoreListen(-1){
	socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

IPv4TCPSocket::IPv4TCPSocket(int socketHandle):restoreTimeout(-1),restoreAddress("0.0.0.0",0),restoreListen(-1){
	this->socketHandle = socketHandle;
}
	
bool IPv4TCPSocket::listen(int maxPendingConnections){
	if(::listen(socketHandle, maxPendingConnections)!=-1){
		restoreListen = maxPendingConnections;
		restoreTimeout = -1;
		return true;
	}
	return false;
}

//! Helper function to do an accept with timeout
static inline int acceptWithTimeout(int socketHandle, uint32_t timeout, sockaddr* saddr, socklen_t* saddrLen){
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(socketHandle, &readSet);
	timeval tv;
	tv.tv_sec=timeout/1000;
	tv.tv_usec=(timeout%1000)*1000;
	if(select(socketHandle+1, &readSet, NULL, NULL, &tv)>0){
		return ::accept(socketHandle, saddr, saddrLen);
	}else{
		return -1;
	}
}

template<typename TSocket, typename TIPAddress>
static inline TSocket* acceptWithTimeout(TSocket* socket, uint32_t timeout, TIPAddress* peerAddress){
	socklen_t internalLen = sizeof(peerAddress->getInternalRepresentation());
	int newHandle = acceptWithTimeout(socket->getSocketHandle(), timeout, (sockaddr*)(peerAddress?&(peerAddress->getInternalRepresentation()):NULL), peerAddress?&internalLen:NULL);
	if(newHandle>=0){
		return new TSocket(newHandle);
	}
	return NULL;
}

IPv4TCPSocket* IPv4TCPSocket::accept(uint32_t timeout, IPv4Address* peerAddress){
	return acceptWithTimeout<IPv4TCPSocket, IPv4Address>(this, timeout, peerAddress);
}

#if !SIMPLESOCKETS_WIN
static int setBlockingFlag(int fd, bool blocking){
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags<0){return -1;}
	flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
	return fcntl(fd, F_SETFL, flags);
}
#endif

//! Helper function to do a connect with timeout
static int connectWithTimeout(int socketHandle, uint32_t timeout, const sockaddr* addr, int addrLen, bool restoreBlocking){
	ONCONNECT
	#if SIMPLESOCKETS_WIN
	unsigned long mode = 1;//0;//TODO 1;
	if(ioctlsocket(socketHandle, FIONBIO, &mode)!=0){
		handleErrorMessage();
	}
	#else
	if(setBlockingFlag(socketHandle, false)==-1){return -1;}
	#endif
	int res = ::connect(socketHandle, addr, addrLen);
	#if SIMPLESOCKETS_WIN
	if(res==-1){
		if(WSAGetLastError()!=WSAEWOULDBLOCK){return -1;}
	}
	#else
	if(res==-1 && errno!=EINPROGRESS){return -1;}
	#endif
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(socketHandle, &fdset);
	timeval tv;
	tv.tv_sec=timeout/1000;
	tv.tv_usec=(timeout%1000)*1000;
	int so_error = -1;
	if(select(socketHandle+1, NULL, &fdset, NULL, &tv)>0){
		socklen_t len = sizeof(so_error);
		getsockopt(socketHandle, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
	}
	if(restoreBlocking){
		#if SIMPLESOCKETS_WIN
		unsigned long mode = 0;
		if(ioctlsocket(socketHandle, FIONBIO, &mode)!=0){
			handleErrorMessage();
		}
		#else
		if(setBlockingFlag(socketHandle, true)==-1){return -1;}
		#endif
	}
	return so_error;
}
	
bool IPv4TCPSocket::connect(const IPv4Address& address, uint32_t timeout){
	#if SIMPLESOCKETS_WIN
	if(connectWithTimeout(socketHandle, timeout, (const sockaddr*)(&(address.getInternalRepresentation())), sizeof(sockaddr_in), isBlocking)==0){
	#else
	if(connectWithTimeout(socketHandle, timeout, (const sockaddr*)(&(address.getInternalRepresentation())), sizeof(sockaddr_in), true)==0){
	#endif
		restoreTimeout = timeout;
		restoreAddress = address;
		restoreListen = -1;
		return true;
	}else{
		return false;
	}
}

bool IPv4TCPSocket::restore(){
	::close(socketHandle);
	socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bool res = true;
	if(restoreTimeout>=0){
		res = res && connect(restoreAddress, restoreTimeout);
	}else if(restoreListen>=0){
		res = res && listen(restoreListen);
	}
	res = res && IPv4Socket::restore();
	return res;
}

IPv6TCPSocket::IPv6TCPSocket():restoreTimeout(-1),restoreAddress("::",0),restoreListen(-1){
	socketHandle = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	setIPv4ReceptionEnabled(true);
}

IPv6TCPSocket::IPv6TCPSocket(int socketHandle):restoreTimeout(-1),restoreAddress("::",0),restoreListen(-1){
	this->socketHandle = socketHandle;
	setIPv4ReceptionEnabled(true);
}

bool IPv6TCPSocket::restore(){
	::close(socketHandle);
	socketHandle = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	bool res = true;
	if(restoreTimeout>=0){
		res = res && connect(restoreAddress, restoreTimeout);
	}else if(restoreListen>=0){
		res = res && listen(restoreListen);
	}
	res = res && IPv6Socket::restore();
	return res;
}
	
bool IPv6TCPSocket::listen(int maxPendingConnections){
	if(::listen(socketHandle, maxPendingConnections)!=-1){
		restoreListen = maxPendingConnections;
		restoreTimeout = -1;
		return true;
	}
	return false;
}
	
IPv6TCPSocket* IPv6TCPSocket::accept(uint32_t timeout, IPv6Address* peerAddress){
	return acceptWithTimeout<IPv6TCPSocket, IPv6Address>(this, timeout, peerAddress);
}
	
bool IPv6TCPSocket::connect(const IPv6Address& address, uint32_t timeout){
	#if SIMPLESOCKETS_WIN
	if(connectWithTimeout(socketHandle, timeout, (const sockaddr*)(&(address.getInternalRepresentation())), sizeof(sockaddr_in6), isBlocking)==0){
	#else
	if(connectWithTimeout(socketHandle, timeout, (const sockaddr*)(&(address.getInternalRepresentation())), sizeof(sockaddr_in6), true)==0){
	#endif
		restoreTimeout = timeout;
		restoreAddress = address;
		restoreListen = -1;
		return true;
	}
	return false;
}

ASocket* connectSocketForAddressList(const std::list<IIPAddress*>& addressList, uint32_t timeout){
	for(std::list<IIPAddress*>::const_iterator it = addressList.begin(); it != addressList.end(); ++it){
		if((*it)->getIPVersion()==IIPAddress::IPV6){
			IPv6TCPSocket* s = new IPv6TCPSocket();
			if(s->connect(*((IPv6Address*)(*it)), timeout)){
				return s;
			}
			delete s;
		}else{
			IPv4TCPSocket* s = new IPv4TCPSocket();
			if(s->connect(*((IPv4Address*)(*it)), timeout)){
				return s;
			}
			delete s;
		}
	}
	return NULL;
}

static inline void* get_in_addr(struct sockaddr *sa) {
  return sa->sa_family == AF_INET?(void *)&(((sockaddr_in*)sa)->sin_addr):(void*)&(((sockaddr_in6*)sa)->sin6_addr);
}

std::list<IIPAddress*> queryIPAddressesForHostName(std::string hostName, uint16_t portToFill){
	checkAndInitGlobally();
	std::list<IIPAddress*> l;
	addrinfo* result;
	addrinfo* res;
	int error = getaddrinfo(hostName.c_str(), NULL, NULL, &result);
	if(error==0){
		for(res = result; res != NULL; res = res->ai_next){   
			char s[INET6_ADDRSTRLEN];
			if(inet_ntop(res->ai_family, get_in_addr((sockaddr*)res->ai_addr), s, sizeof(s))==NULL){
				handleErrorMessage();
				return l;
			}
			if(res->ai_family==AF_INET){
				l.push_back(new IPv4Address(s, portToFill));
			}else{
				l.push_back(new IPv6Address(s, portToFill));
			}
		}
		freeaddrinfo(result);
	}
	return l;	
}

ASocket* createSocketForHostName(bool tcp, std::string hostName, uint16_t port, uint32_t tcpConnectTimeout){
	ASocket* res = NULL;
	std::list<IIPAddress*> l = queryIPAddressesForHostName(hostName, port);
	if(tcp){
		res = connectSocketForAddressList(l, tcpConnectTimeout);
	}else if(!l.empty()){
		IIPAddress* a = l.front();
		if(a->getIPVersion()==IIPAddress::IPV4){
			IPv4UDPSocket* s = new IPv4UDPSocket();
			s->setUDPTarget(*(IPv4Address*)a);
			res = s;
		}else{
			IPv6UDPSocket* s = new IPv6UDPSocket();
			s->setUDPTarget(*(IPv6Address*)a);
			res = s;
		}
	}
	for(IIPAddress* a : l){delete a;}
	return res;
}

std::list<IPv4Address> queryIPv4BroadcastAdresses(uint16_t portToFill){
	checkAndInitGlobally();
	std::list<IPv4Address> l;
	IPv4Address address("255.255.255.255", portToFill);
	#if SIMPLESOCKETS_WIN
	PMIB_IPADDRTABLE ipAddrTable = (MIB_IPADDRTABLE*)malloc(sizeof(MIB_IPADDRTABLE));
	if(ipAddrTable!=NULL){
		DWORD size = 0;
		if(GetIpAddrTable(ipAddrTable, &size, 0)==ERROR_INSUFFICIENT_BUFFER){//first call: get the required size
			free(ipAddrTable);
			ipAddrTable = (MIB_IPADDRTABLE*)malloc(size);
		}
		if(ipAddrTable!=NULL){
			DWORD retVal;
			if((retVal = GetIpAddrTable(ipAddrTable, &size, 0))!=NO_ERROR){
				std::cerr << "GetIpAddrTable failed with error: " << retVal << std::endl;
				LPVOID msgBuf;
				if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, retVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & msgBuf, 0, NULL)){
					std::cerr << msgBuf << std::endl;
					LocalFree(msgBuf);
				}
			}else{
				for(int i=0; i<(int)ipAddrTable->dwNumEntries; i++){
					sockaddr_in addr;
					addr.sin_addr.S_un.S_addr = ((uint32_t)ipAddrTable->table[i].dwAddr) | ((~(uint32_t)ipAddrTable->table[i].dwMask) & (~(uint32_t)0)); //calculate real broadcast address from subnet mask and ip address (the one returned by windows is wrong)
					addr.sin_port = htons(portToFill);
					addr.sin_family = AF_INET;
					address.setInternalRepresentation(addr);
					l.push_back(address);
				}
			}
		}
	}
	free(ipAddrTable);
	#elif defined(USE_ANDROID_BRD_ADDR_WORKAROUND)//works on Linux and Android
	static const size_t len = sizeof(ifreq);
	char buf[16384];
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	ifconf ifconf;
	ifconf.ifc_len = sizeof(buf);
	ifconf.ifc_buf = buf;
	if(ioctl(sock, SIOCGIFCONF, &ifconf)==0){
		ifreq* interface = ifconf.ifc_req;
		for(int i=0; i<ifconf.ifc_len; i += len){
			if(ioctl(sock, SIOCGIFBRDADDR, interface)==0){
				if(interface->ifr_broadaddr.sa_family==AF_INET){
					sockaddr_in addr = *(sockaddr_in*)(&interface->ifr_broadaddr);
					if(addr.sin_addr.s_addr!=0){
						addr.sin_port = htons(portToFill);
						address.setInternalRepresentation(addr);
						l.push_back(address);
					}
					interface = (ifreq*)((char*)interface + len);
				}
			}
		}
	}
	close(sock);
	#else//works on Linux and should work on MacOS, iOS
	ifaddrs* ifap;
	if(getifaddrs(&ifap) == 0){
		ifaddrs* p = ifap;
		while(p){
			if((p->ifa_flags&IFF_BROADCAST)!=0 && p->ifa_addr->sa_family==AF_INET && p->ifa_broadaddr->sa_family==AF_INET){
				sockaddr_in internal = *(sockaddr_in*)p->ifa_broadaddr;
				internal.sin_port = htons(portToFill);
				address.setInternalRepresentation(internal);
				l.push_back(address);
			}
			p = p->ifa_next;
		}
		freeifaddrs(ifap);
	}
	#endif
    //for(auto it=l.begin(); it!=l.end(); ++it){std::cout << it->getAddressAsString() << std::endl;}
	if(l.empty()){
		l.push_back(address);//push back 255.255.255.255 as fallback
	}
	return l;
}
