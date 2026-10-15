#ifndef UCIPV4ADDRESS_H
#define UCIPV4ADDRESS_H

#include <stdint.h>

//! A simple ipv4 address and port representation, needed since different stacks use different representations, so this provides a unified way to handle IP addresses and ports.
class uCIPv4Address{

	public:

	uint8_t octets[4];
	uint16_t port;

	//! Byte order shall be o0.o1.o2.o3
	uCIPv4Address(uint8_t o0, uint8_t o1, uint8_t o2, uint8_t o3, uint16_t p){
		octets[0] = o0;
		octets[1] = o1;
		octets[2] = o2;
		octets[3] = o3;
		port = p;
	}

	uCIPv4Address(const uint8_t o[4], uint16_t p){
		octets[0] = o[0];
		octets[1] = o[1];
		octets[2] = o[2];
		octets[3] = o[3];
		port = p;
	}


	uCIPv4Address(): octets{0,0,0,0}, port(0){}

	uint32_t toUInt32() const{
		return (static_cast<uint32_t>(octets[0]) << 24) |
		       (static_cast<uint32_t>(octets[1]) << 16) |
		       (static_cast<uint32_t>(octets[2]) << 8)  |
		       (static_cast<uint32_t>(octets[3]));
	}

	uCIPv4Address(uint32_t ip, uint16_t p){
		octets[0] = (ip >> 24) & 0xFF;
		octets[1] = (ip >> 16) & 0xFF;
		octets[2] = (ip >> 8) & 0xFF;
		octets[3] = ip & 0xFF;
		port = p;
	}

	uint8_t operator[](uint8_t index) const {
		return octets[index];
	}
};

#endif