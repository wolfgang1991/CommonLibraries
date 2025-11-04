#include "RTPSender.h"

#include <ICommunicationEndpoint.h>
#include <BitFunctions.h>

#include <iostream>

class RTPSenderPrivate{

	public:
	
	ICommunicationEndpoint* slaveSocket;
	uint8_t payloadType;
	uint32_t mps;
	uint16_t sequenceNumber;
	uint32_t ssrc;
	bool mustDelete;
	
	RTPSenderPrivate(ICommunicationEndpoint* slaveSocket, uint8_t payloadType, uint32_t ssrc, uint32_t maxPacketSize, bool mustDelete):slaveSocket(slaveSocket),payloadType(payloadType),mps(maxPacketSize - 12),ssrc(ssrc),mustDelete(mustDelete){
		sequenceNumber = 0;
	}
	
	~RTPSenderPrivate(){
		if(mustDelete){
			delete slaveSocket;
		}
	}
	
	void writeHeader(uint8_t* buf, bool marker, uint32_t timestamp){
		buf[0] = 0b10000000;
		buf[1] = setBit(payloadType, 7, marker);
		uint8_t offset = 2;
		writeBigEndian<uint16_t>(buf, offset, sequenceNumber);
		sequenceNumber++;
		writeBigEndian<uint32_t>(buf, offset, timestamp);
		writeBigEndian<uint32_t>(buf, offset, ssrc);
	}
	
	bool send(uint8_t* buf, uint32_t totalLength, uint32_t timestamp){
		uint32_t fragmentCount = 0;
		uint8_t saved[12];
		for(uint32_t offset = 12; offset < totalLength; offset += mps){
			uint8_t* ptr = &(buf[offset-12]);
			memcpy(saved, ptr, 12);
			writeHeader(ptr, offset+mps>=totalLength, timestamp);
			slaveSocket->send((char*)ptr, 12+std::min(totalLength-offset, mps));
			memcpy(ptr, saved, 12);
			fragmentCount++;
		}
		//std::cout << "fragmentCount: " << fragmentCount << std::endl;
		return true;
	}
	
};

RTPSender::RTPSender(ICommunicationEndpoint* slaveSocket, uint8_t payloadType, uint32_t ssrc, uint32_t maxPacketSize, bool mustDelete){
	p = new RTPSenderPrivate(slaveSocket, payloadType, ssrc, maxPacketSize, mustDelete);
}
	
RTPSender::~RTPSender(){
	delete p;
}

bool RTPSender::send(uint8_t* buf, uint32_t totalLength, uint32_t timestamp){
	return p->send(buf, totalLength, timestamp);
}
