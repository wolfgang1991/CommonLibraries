#ifndef MJPEGRTPPayloadCreator_H_
#define MJPEGRTPPayloadCreator_H_

#include <RTPSender.h>

#include <cstdint>

//! Payload is according to RFC2435 and can be used with RTPSender
struct MJPEGRTPPayload{
	uint8_t* buf;
	uint32_t totalLength;//! at least RTPSender::headerSize, or 0 if unallocated yet
	uint32_t bufferLength;//! total allocated buffer length, must be at least totalLength, can be larger for future use
	uint32_t packetPayloadSize;//! payload bytes per RTP packet, excluding the 12-byte RTP header

	MJPEGRTPPayload():buf(nullptr), totalLength(0), bufferLength(0), packetPayloadSize(1400-RTPSender::headerSize){}

};

//! according to RFC2435
//! payload reallocated if to small or nullptr
//! jpegBuf and jpegSize are a full baseline sequential jpeg image as created by tjCompress2
//! unsupported jpeg variants leave payload.totalLength == 0
void createMJPEGRTPPayload(MJPEGRTPPayload& payload, uint8_t* jpegBuf, uint32_t jpegSize);

#endif