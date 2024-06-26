#ifndef RTPSender_H_
#define RTPSender_H_

#include <cstdint>

class RTPSenderPrivate;
class ICommunicationEndpoint;

class RTPSender{

	RTPSenderPrivate* p;
	
	public:
	
	static constexpr uint32_t headerSize = 12;
	
	//! payloadType field in header, ssrc unique identifier for this source
	//! maxPacketSize should be the lowest MTU on the network minus UDP overhead
	//! mustDelete: true if slaveSocket must be deleted on destruction
	RTPSender(ICommunicationEndpoint* slaveSocket, uint8_t payloadType, uint32_t ssrc, uint32_t maxPacketSize, bool mustDelete = true);
	
	~RTPSender();
	
	//! buf WILL BE CHANGED, data may not be usable afterwards
	//! assumes the first bytes of headerSize are not used such that the header can be filled
	//! first packet will have a marker set
	//! totalLength = payload length + headerSize
	bool send(uint8_t* buf, uint32_t totalLength, uint32_t timestamp);
	
};

#endif
