#ifndef ZSocket_H_INCLUDED
#define ZSocket_H_INCLUDED

#ifndef NO_ZLIB_SUPPORT

#include "SimpleSockets.h"

class ZSocketPrivate;

//! Socket Layer for transparent compression with zlib using a "slave" socket
class ZSocket : public ISocket{
	friend class ZSocketPrivate;

	private:
	
	ZSocketPrivate* p;

	public:
	
	//! mustDeleteSlaveSocket: true if slaveSocket shall be deleted upon destruction, sendBufSize/recvBufSize: buffer size for compressed data, should be large to get as much data as possible with less overhead
	ZSocket(ISocket* slaveSocket, uint32_t sendBufSize = 1024*1024, uint32_t recvBufSize = 1024*1024, uint32_t compressionLevel = 9, bool mustDeleteSlaveSocket = true);
	
	~ZSocket();
	
	//! returns the available compressed bytes for reading
	uint32_t getAvailableBytes() const;
	
	uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false);
	
	bool send(const char* buf, uint32_t bufSize);
	
};

#endif

#endif
