#ifndef NamedPipes_H_INCLUDED
#define NamedPipes_H_INCLUDED

#include "SimpleSockets.h"

#ifdef SIMPLESOCKETS_WIN

//! Common implementations for NamedPipeServer and NamedPipeClient
class ANamedPipe : public ISocket{
	
	protected:
	
	HANDLE hPipe;
	mutable bool connected;
	
	public:
	
	ANamedPipe();
	
	virtual ~ANamedPipe(){}
	
	uint32_t getAvailableBytes() const;
	
	uint32_t recv(char* buf, uint32_t bufSize, bool readBlocking = false);
	
	bool send(const char* buf, uint32_t bufSize);
	
	bool isConnected() const;
	
};

//! Server part of a named pipe (the part which creates the pipe and usually waits for a client)
class NamedPipeServer : public ANamedPipe{
	
	private:
	
	std::string namedPipe;
	bool blocking;
	bool byteOriented;
	uint32_t bufSize;
	
	void recreate();
		
	public:
	
	//! byteOriented: true if stream of bytes, false if stream of messages
	NamedPipeServer(const std::string& namedPipe, bool blocking, bool byteOriented = true, uint32_t bufSize = 1024);
	
	~NamedPipeServer();
	
	//! must be called by the server application to check if a client has been connected (non blocking mode) or wait until a client connect (blocking mode)
	//! returns true if the pipe is connected
	bool connect();
	
};

class NamedPipeClient : public ANamedPipe{
	
	public:
	
	NamedPipeClient();
	
	~NamedPipeClient();
	
	//! true if successful
	bool connect(const std::string& namedPipe);
	
};


#endif

#endif
