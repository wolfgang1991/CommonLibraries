#include "ConcurrentCommunicationEndpoint.h"

#include <Threading.h>
#include <timing.h>

#include <cassert>
#include <cstring>

class ConcurrentCommunicationEndpointPrivate{

	public:
	
	struct BufferWithOffset{
		uint8_t* buf;
		uint32_t offset;
		uint32_t size;
		
		BufferWithOffset(uint32_t size):buf(new uint8_t[size]),offset(0),size(size){}
		
		~BufferWithOffset(){
			delete[] buf;
		}
		
		//! copy content of other buffer with overwriting old data if necessary
		void append(BufferWithOffset* other){
			assert(offset+other->offset<=size);
			memcpy(&(buf[offset]), other->buf, other->offset);
			offset += other->offset;
		}
		
	};
	
	Thread t;
	ICommunicationEndpoint* endpoint;
	BufferWithOffset* sendBuffer;
	BufferWithOffset* rcvBuffer;
	
	Mutex m;
	bool mustExit;
	BufferWithOffset* sendBufferExchange;
	BufferWithOffset* rcvBufferExchange;
	uint32_t rcvBufferStartOffset;//where to start with reading
	
	static void* cceMain(void* param){
		ConcurrentCommunicationEndpointPrivate* prv = (ConcurrentCommunicationEndpointPrivate*)param;
		bool running = true;
		while(running){
			//exchange buffers or contents
			lockMutex(prv->m);
			running = !prv->mustExit;
			BufferWithOffset* tmp = prv->sendBuffer;
			prv->sendBuffer = prv->sendBufferExchange;
			prv->sendBufferExchange = tmp;
			prv->sendBufferExchange->offset = 0;
			if(prv->rcvBufferExchange->offset==0 || prv->rcvBufferExchange->offset+prv->rcvBuffer->offset>prv->rcvBufferExchange->size){
				tmp = prv->rcvBuffer;
				prv->rcvBuffer = prv->rcvBufferExchange;
				prv->rcvBufferExchange = tmp;
				prv->rcvBufferStartOffset = 0;
			}else{
				prv->rcvBufferExchange->append(prv->rcvBuffer);
			}
			unlockMutex(prv->m);
			prv->rcvBuffer->offset = 0;
			//send
			if(prv->sendBuffer->offset>0){
				prv->endpoint->send((char*)prv->sendBuffer->buf, prv->sendBuffer->offset);
				prv->sendBuffer->offset = 0;
			}
			//receive
			int32_t received = prv->endpoint->recv((char*)prv->rcvBuffer->buf, prv->rcvBuffer->size);
			if(received>=0){
				prv->rcvBuffer->offset = received;
			}
			delay(1);
		}
		return NULL;
	}

};

ConcurrentCommunicationEndpoint::ConcurrentCommunicationEndpoint(ICommunicationEndpoint* endpoint, uint32_t sendBufSize, uint32_t rcvBufSize){
	prv = new ConcurrentCommunicationEndpointPrivate();
	prv->endpoint = endpoint;
	prv->sendBuffer = new ConcurrentCommunicationEndpointPrivate::BufferWithOffset(sendBufSize);
	prv->rcvBuffer = new ConcurrentCommunicationEndpointPrivate::BufferWithOffset(rcvBufSize);
	prv->sendBufferExchange = new ConcurrentCommunicationEndpointPrivate::BufferWithOffset(sendBufSize);
	prv->rcvBufferExchange = new ConcurrentCommunicationEndpointPrivate::BufferWithOffset(rcvBufSize);
	prv->rcvBufferStartOffset = 0;
	initMutex(prv->m);
	prv->mustExit = false;
	bool res = createThread(prv->t, ConcurrentCommunicationEndpointPrivate::cceMain, prv, true);
	assert(res);
}
	
ConcurrentCommunicationEndpoint::~ConcurrentCommunicationEndpoint(){
	lockMutex(prv->m);
	prv->mustExit = true;
	unlockMutex(prv->m);
	bool success = joinThread(prv->t);//TODO abandon if blocked forever?
	assert(success);
	deleteMutex(prv->m);
	delete prv->sendBuffer;
	delete prv->rcvBuffer;
	delete prv->sendBufferExchange;
	delete prv->rcvBufferExchange;
	delete prv;
}
	
int32_t ConcurrentCommunicationEndpoint::recv(char* buf, uint32_t bufSize){
	lockMutex(prv->m);
	uint32_t toRead = prv->rcvBufferExchange->offset-prv->rcvBufferStartOffset;
	if(toRead>bufSize){toRead = bufSize;}
	if(toRead>0){
		memcpy(buf, &(prv->rcvBufferExchange->buf[prv->rcvBufferStartOffset]), toRead);
		prv->rcvBufferStartOffset += toRead;
		if(prv->rcvBufferStartOffset>=prv->rcvBufferExchange->offset){
			prv->rcvBufferStartOffset = prv->rcvBufferExchange->offset = 0;
		}
	}
	unlockMutex(prv->m);
	return toRead;
}
	
bool ConcurrentCommunicationEndpoint::send(const char* buf, uint32_t bufSize){
	lockMutex(prv->m);
	if(prv->sendBufferExchange->offset+bufSize<prv->sendBufferExchange->size){
		memcpy(&prv->sendBufferExchange->buf[prv->sendBufferExchange->offset], buf, bufSize);
		prv->sendBufferExchange->offset += bufSize;
	}else{
		memcpy(prv->sendBufferExchange->buf, buf, bufSize);
		prv->sendBufferExchange->offset = bufSize;
	}
	unlockMutex(prv->m);
	return true;
}
