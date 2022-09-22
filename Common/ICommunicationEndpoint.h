#ifndef ICommunicationEndpoint_H_INCLUDED
#define ICommunicationEndpoint_H_INCLUDED

#include <cstdint>

//! interface for endpoints of communications (may be network sockets, serial port, bluetooth, ...)
//! establishing a connection is part of the underlying protocol
class ICommunicationEndpoint{
	
	public:
	
	virtual ~ICommunicationEndpoint(){}
	
	//! should be a non blocking read, but it's not guranteed, returns the number of bytes read or -1 on error
	virtual int32_t recv(char* buf, uint32_t bufSize) = 0;
	
	//! true if buf has been sent (does not gurantee reception on other side)
	virtual bool send(const char* buf, uint32_t bufSize) = 0;
	
};

#endif
