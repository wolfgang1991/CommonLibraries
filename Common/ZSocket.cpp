#ifndef NO_ZLIB_SUPPORT
#include "ZSocket.h"

#include <zlib.h>
#include <cassert>
#include <iostream>

class ZSocketPrivate{
	
	public:
	
	bool mustDeleteSlaveSocket;
	ISocket* slaveSocket;
	
	char* sendBuf;
	uint32_t sendBufSize;
	
	char* recvBuf;
	uint32_t recvBufSize;
	
	z_stream deflateStrm;
	
	z_stream inflateStrm;
	bool lastInflateFinished;
	
	void initInflate(){
		//allocate inflate state
		inflateStrm.zalloc = Z_NULL;
		inflateStrm.zfree = Z_NULL;
		inflateStrm.opaque = Z_NULL;
		inflateStrm.avail_in = 0;
		inflateStrm.next_in = Z_NULL;
		int ret = inflateInit(&inflateStrm);
		assert(ret==Z_OK);
		lastInflateFinished = true;
	}
	
	ZSocketPrivate(ISocket* slaveSocket, uint32_t sendBufSize, uint32_t recvBufSize, uint32_t compressionLevel, bool mustDeleteSlaveSocket):mustDeleteSlaveSocket(mustDeleteSlaveSocket),slaveSocket(slaveSocket),sendBufSize(sendBufSize),recvBufSize(recvBufSize){
		sendBuf = new char[sendBufSize];
		recvBuf = new char[recvBufSize];
		//allocate deflate state
		deflateStrm.zalloc = Z_NULL;
		deflateStrm.zfree = Z_NULL;
		deflateStrm.opaque = Z_NULL;
		int ret = deflateInit(&deflateStrm, compressionLevel);
		assert(ret==Z_OK);
		initInflate();
	}
	
	~ZSocketPrivate(){
		if(mustDeleteSlaveSocket){delete slaveSocket;}
		deflateEnd(&deflateStrm);
		inflateEnd(&inflateStrm);
		delete[] sendBuf;
		delete[] recvBuf;
	}
	
	bool send(const char* inBuf, uint32_t inBufSize){
		bool success = true;
		if(inBufSize>0){
			deflateStrm.avail_in = inBufSize;
			deflateStrm.next_in = (Bytef*)inBuf;
			do{
				deflateStrm.avail_out = sendBufSize;
				deflateStrm.next_out = (Bytef*)sendBuf;
				int ret = deflate(&deflateStrm, Z_SYNC_FLUSH);
				assert(ret != Z_STREAM_ERROR);
				uint32_t have = sendBufSize - deflateStrm.avail_out;//available bytes, which is the difference between how much space was provided before the call, and how much output space is still available after the call.
				if(have>0){
					bool thisSuccess = slaveSocket->send(sendBuf, have);
					success = success && thisSuccess;
				}
		    }while(deflateStrm.avail_out==0);
		}
		return success;
	}
	
	uint32_t execInflate(char* outBuf, uint32_t outBufSize){
		inflateStrm.avail_out = outBufSize;
    	inflateStrm.next_out = (Bytef*)outBuf;
    	int ret = inflate(&inflateStrm, Z_SYNC_FLUSH);
    	assert(ret != Z_STREAM_ERROR);
    	if(ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR){
    		std::cerr << "ERROR: zlib inflate error: " << ret << std::endl;
    		inflateEnd(&inflateStrm);
    		initInflate();
    		return 0;
    	}else{
    		lastInflateFinished = inflateStrm.avail_out!=0;
    		return outBufSize-inflateStrm.avail_out;
    	}
	}
	
	uint32_t recv(char* outBuf, uint32_t outBufSize, bool readBlocking){
		if(!lastInflateFinished){
			return execInflate(outBuf, outBufSize);
		}
		uint32_t received = slaveSocket->recv(recvBuf, recvBufSize, readBlocking);
		if(received>0){
			inflateStrm.avail_in = received;
			inflateStrm.next_in = (Bytef*)recvBuf;
			return execInflate(outBuf, outBufSize);
		}
		return 0;
	}
	
};

ZSocket::ZSocket(ISocket* slaveSocket, uint32_t sendBufSize, uint32_t recvBufSize, uint32_t compressionLevel, bool mustDeleteSlaveSocket){
	p = new ZSocketPrivate(slaveSocket, sendBufSize, recvBufSize, compressionLevel, mustDeleteSlaveSocket);
}
	
ZSocket::~ZSocket(){
	delete p;
}

uint32_t ZSocket::getAvailableBytes() const{
	return p->slaveSocket->getAvailableBytes();
}

uint32_t ZSocket::recv(char* buf, uint32_t bufSize, bool readBlocking){
	return p->recv(buf, bufSize, readBlocking);
}

bool ZSocket::send(const char* buf, uint32_t bufSize){
	return p->send(buf, bufSize);
}

#endif
