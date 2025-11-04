#ifndef RTPReceiver_H_
#define RTPReceiver_H_

#include "ICommunicationEndpoint.h"

#include <cstdint>
#include <map>

class FrameBasedRTPReceiverPrivate;

//! assumes marked is set to mark a frame's end
class FrameBasedRTPReceiver{

	FrameBasedRTPReceiverPrivate* p;
	
	public:
	
	struct Buffer{
		static constexpr uint32_t size = 65536;
		char data[size];
		uint32_t start;//index of first byte after header
		uint32_t used;//used bytes after header
		bool marker;
		uint8_t payloadType;
		uint16_t sequenceNumber;
		uint32_t timestamp;

		Buffer();

		void reset();
	};

	struct Frame{
		uint16_t startSequenceNumber = 0;
		uint16_t endSequenceNumber = 0;
		bool endValid = false;
		std::map<uint16_t, Buffer*> seqNr2Packet;

		Frame(uint16_t sequenceNumber):startSequenceNumber(sequenceNumber){}

		void setStart(uint16_t sequenceNumber);

		void setEnd(uint16_t sequenceNumber);

		bool isFullyAssembled() const;

		bool isInside(uint16_t sequenceNumber) const;
	};
	
	//! slaveSocket must be packet oriented, mustDelete: true if slaveSocket must be deleted on destruction
	FrameBasedRTPReceiver(ICommunicationEndpoint* slaveSocket, bool mustDelete = true, uint32_t maxPendingFrames = 5, uint32_t maxPacketsPerFrame = 500);
	
	virtual ~FrameBasedRTPReceiver();
	
	//! returns NULL if no complete frame is available
	const Frame* update();
	
};

#endif
