#ifndef ConcurrentCommunicationEndpoint_H_INCLUDED
#define ConcurrentCommunicationEndpoint_H_INCLUDED

#include "ICommunicationEndpoint.h"

class ConcurrentCommunicationEndpointPrivate;

//! A separate thread is used for acutal send/recv to avoid blocking problems.
//! This is meant to be used with protocols where data loss is acceptable such as UDP or serial port communication
//! If the send/recv thread takes too long there may be data loss.
//! The sendBufSize and rcvBufSize should be at least 2x the largest expected packet.
class ConcurrentCommunicationEndpoint : public ICommunicationEndpoint{
	
	private:
	
	ConcurrentCommunicationEndpointPrivate* prv;
	
	public:
	
	static constexpr uint32_t defaultBufSize = 3264; //1048576;//1MiB
	
	//! endpoint must no longer be used in this thread (other threads may access it at any time while this exists)
	ConcurrentCommunicationEndpoint(ICommunicationEndpoint* endpoint, uint32_t sendBufSize = defaultBufSize, uint32_t rcvBufSize = defaultBufSize);
	
	~ConcurrentCommunicationEndpoint();
	
	int32_t recv(char* buf, uint32_t bufSize);
	
	bool send(const char* buf, uint32_t bufSize);
	
};

#endif
