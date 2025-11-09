#include "RTPReceiver.h"

#include <BitFunctions.h>
#include <misc.h>

#include <list>
#include <iostream>

void FrameBasedRTPReceiver::Frame::setStart(uint16_t sequenceNumber){
	startSequenceNumber = sequenceNumber;
}
		
void FrameBasedRTPReceiver::Frame::setEnd(uint16_t sequenceNumber){
	endSequenceNumber = sequenceNumber;
	endValid = true;
}

bool FrameBasedRTPReceiver::Frame::isInside(uint16_t sequenceNumber) const{
	if(!endValid){//end unknown
		return sequenceNumber>=startSequenceNumber;
	}else if(startSequenceNumber<=endSequenceNumber){
		return sequenceNumber>=startSequenceNumber && sequenceNumber<=endSequenceNumber;
	}else{//overflow
		return sequenceNumber>=startSequenceNumber || sequenceNumber<=endSequenceNumber;
	}
}

bool FrameBasedRTPReceiver::Frame::isFullyAssembled() const{
	return endValid && seqNr2Packet.size()>=(uint32_t)(endSequenceNumber-startSequenceNumber+1);
}

FrameBasedRTPReceiver::Buffer::Buffer(){
	reset();
}

void FrameBasedRTPReceiver::Buffer::reset(){
	start = 0;
	used = 0;
	marker = false;
	payloadType = 0;
	sequenceNumber = 0;
	timestamp = 0;
}

class FrameBasedRTPReceiverPrivate{
	
	public:
	
	using Buffer = FrameBasedRTPReceiver::Buffer;
	using Frame = FrameBasedRTPReceiver::Frame;
	
	ICommunicationEndpoint* slaveSocket;
	
	const bool mustDelete;
	const uint32_t maxPendingFrames;
	const uint32_t maxPacketsPerFrame;

	std::list<Frame>::iterator toRemove;
	std::list<Frame> frames;
	Buffer* rcvBuf;

	std::list<Buffer*> usedBuffers;

	FrameBasedRTPReceiverPrivate(ICommunicationEndpoint* slaveSocket, bool mustDelete, uint32_t maxPendingFrames, uint32_t maxPacketsPerFrame):slaveSocket(slaveSocket),mustDelete(mustDelete),maxPendingFrames(maxPendingFrames),maxPacketsPerFrame(maxPacketsPerFrame){
		rcvBuf = new Buffer();
		toRemove = frames.end();
	}
	
	~FrameBasedRTPReceiverPrivate(){
		if(mustDelete){
			delete slaveSocket;
		}
		delete rcvBuf;
		for(auto b: usedBuffers){
			delete b;
		}
	}

	Buffer* getBuffer(){
		if(!usedBuffers.empty()){
			Buffer* b = usedBuffers.front();
			usedBuffers.pop_front();
			b->reset();
			return b;
		}else{
			return new Buffer();
		}
	}

	void returnBuffer(Buffer* b){
		usedBuffers.push_back(b);
	}

	//! returns frame where inserted or end
	std::list<Frame>::iterator insertInFrames(Buffer* b){
		std::list<Frame>::iterator res = frames.end();
		for(auto it = frames.begin(); it!=frames.end(); it++){
			if(it->isInside(b->sequenceNumber)){//found
				if(it->seqNr2Packet.find(b->sequenceNumber)!=it->seqNr2Packet.end()){
					std::cerr << "Duplicate packet, dropping." << std::endl;
				}else{
					it->seqNr2Packet[b->sequenceNumber] = b;
					res = it;
					if(b->marker){
						it->setEnd(b->sequenceNumber);
					}else if(it->seqNr2Packet.size()>maxPacketsPerFrame){
						removeFrame(it);
						std::cerr << "Frame too large, dropping." << std::endl;
						res = frames.end();
					}
				}
				break;
			}
		}
		if(b->marker){
			//Create new frame for next packets
			uint16_t nextSeqNr = b->sequenceNumber+1;
			Frame* nextFrame = nullptr;
			for(auto it = frames.begin(); it!=frames.end() && !nextFrame; it++){
				if(it->startSequenceNumber==nextSeqNr){nextFrame = &(*it);}
			}
			if(!nextFrame){
				frames.emplace_back(nextSeqNr);
				nextFrame = &frames.back();
			}
			//Insert wrongly inserted into new frame
			if(res!=frames.end()){
				auto sit = res->seqNr2Packet.find(b->sequenceNumber);
				while(sit!=res->seqNr2Packet.end()){
					if(sit->second->sequenceNumber>=nextSeqNr){
						nextFrame->seqNr2Packet[sit->second->sequenceNumber] = sit->second;
						sit = res->seqNr2Packet.erase(sit);
					}else{
						sit++;
					}
				}
			}
		}
		return res;
	}

	void removeOlderFrames(std::list<Frame>::iterator upto){
		auto it = frames.begin();
		while(it!=upto){
			it = removeFrame(it);
		}
	}

	std::list<Frame>::iterator removeFrame(std::list<Frame>::iterator it){
		for(auto p : it->seqNr2Packet){
			returnBuffer(p.second);
		}
		return frames.erase(it);
	}
	
	//! parses header and fills fields
	bool parseHeader(Buffer* b, uint32_t received){
		if(received>12){
			char* data = b->data;
			//hex_dump(std::cout, data, received);
			uint8_t version = (data[0] >> 6) & 0b11;
			if(version==2){
				bool hasPadding = getBit(data[0], 5);
				if(!hasPadding){
					bool hasExtension = getBit(data[0], 4);
					if(!hasExtension){
						b->marker = getBit(data[1], 7);
						b->payloadType = data[1] & 0b01111111;
						uint32_t offset = 2;
						b->sequenceNumber = readBigEndian<uint16_t>((uint8_t*)data, offset);
						//std::cout << "sequenceNumber: " << b->sequenceNumber << std::endl;
						b->timestamp = readBigEndian<uint32_t>((uint8_t*)data, offset);
						uint8_t cc = data[0] & 0b1111;
						b->start = 12+4*cc;
						b->used = received-b->start;
						return true;
					}else{
						std::cerr << "Extension not (yet) supported." << std::endl;
					}
				}else{
					std::cerr << "Padding not (yet) supported." << std::endl;
				}
			}else{
				std::cerr << "Bad RTP Version: " << (int)version << std::endl;
			}
		}else{
			std::cerr << "Header too short: " << received << std::endl;
		}
		return false;
	};
	
	const Frame* update(uint32_t* totalReceived){
		//remove previous frame
		if(toRemove!=frames.end()){
			removeFrame(toRemove);
			toRemove = frames.end();
		}
		//receive
		uint32_t received = slaveSocket->recv(rcvBuf->data, Buffer::size);
		uint32_t sum = 0;
		while(received>0){
			sum += received;
			if(parseHeader(rcvBuf, received)){
				//check too many frames
				if(frames.size()>=maxPendingFrames){
					removeFrame(frames.begin());
					std::cerr << "Too many pending frames, dropping oldest." << std::endl;
				}
				//insert + check if done + return if not inserted
				std::list<Frame>::iterator it = insertInFrames(rcvBuf);
				if(it!=frames.end()){
					rcvBuf = getBuffer();
					if(it->isFullyAssembled()){
						const Frame* res = &(*it);
						removeOlderFrames(it);
						toRemove = it;
						if(totalReceived){*totalReceived = sum;}
						return res;
					}
				}
			}
			rcvBuf->reset();
			received = slaveSocket->recv(rcvBuf->data, Buffer::size);
		}
		if(totalReceived){*totalReceived = sum;}
		return NULL;
	}
	
};


FrameBasedRTPReceiver::FrameBasedRTPReceiver(ICommunicationEndpoint* slaveSocket, bool mustDelete, uint32_t maxPendingFrames, uint32_t maxPacketsPerFrame){
	p = new FrameBasedRTPReceiverPrivate(slaveSocket, mustDelete, maxPendingFrames, maxPacketsPerFrame);
}
	
FrameBasedRTPReceiver::~FrameBasedRTPReceiver(){
	delete p;
}

const FrameBasedRTPReceiver::Frame* FrameBasedRTPReceiver::update(uint32_t* totalReceived){
	return p->update(totalReceived);
}