#ifndef uCMultiUDPSocket_H
#define uCMultiUDPSocket_H

#include "IUCUDPSocket.h"
#include "uCIPv4Address.h"
#include "uCArray.h"

struct UCRoutingTableEntry{

	uint32_t address;
	uint32_t subnetMask;
	uint8_t prefixLength;
	IUCUDPSocket* socket;

	UCRoutingTableEntry(uCIPv4Address address, uint8_t prefixLength, IUCUDPSocket* socket)
		: prefixLength(prefixLength), socket(socket) {
		subnetMask = (prefixLength == 0) ? 0 : (0xFFFFFFFF << (32 - prefixLength));
		this->address = address.toUInt32() & subnetMask;
	}

	bool matches(uint32_t ipAddress) const {
		return (ipAddress & subnetMask) == address;
	}
	
	bool matches(uCIPv4Address ipAddress) const {
		return matches(ipAddress.toUInt32());
	}

	uint32_t getBroadcastAddress() const {
		return address | ~subnetMask;
	}

	uCIPv4Address getBroadcastAddressAsIPv4Address(uint16_t port = 0) const {
		return uCIPv4Address(getBroadcastAddress(), port);
	}

	bool operator<(const UCRoutingTableEntry& other) const {
		return prefixLength > other.prefixLength;//longest prefix match: longer prefixes come first
	}

};

//! Unlike on desktop OSes a socket may be valid for one specific stack only. For multiple stacks they can be combined using this class (e.g. LwIP and W5500)
//! routing table must only contain each socket once
template <uint8_t socketCount>
class UCMultiUDPSocket : public IUCUDPSocket {

	ucstd::array<UCRoutingTableEntry, socketCount>& routingTable;
	uCIPv4Address currentTarget;

	uint8_t lastUsedSocketIndex = 0;
	uCIPv4Address lastDatagramAddress;

	static_assert(socketCount > 0, "UCMultiUDPSocket must have at least one socket");

	public:

	UCMultiUDPSocket(ucstd::array<UCRoutingTableEntry, socketCount>& routingTable):routingTable(routingTable){
		routingTable.sort();
	}

	~UCMultiUDPSocket() override = default;

	void setUDPTarget(const uCIPv4Address& target){
		currentTarget = target;
	}

	uCIPv4Address getLastDatagramAddress(){
		return lastDatagramAddress;
	}

	bool send(const char* buf, uint32_t bufSize){
		uint32_t targetIP = currentTarget.toUInt32();
		if(targetIP == 0xFFFFFFFF) {//broadcast
			for(uint8_t i = 0; i < routingTable.size(); ++i) {
				UCRoutingTableEntry& e = routingTable[i];
				e.socket->setUDPTarget(e.getBroadcastAddressAsIPv4Address(currentTarget.port));
				e.socket->send(buf, bufSize);
			}
			return true;
		}else{
			for(uint8_t i = 0; i < routingTable.size(); ++i) {
				UCRoutingTableEntry& e = routingTable[i];
				if(e.matches(targetIP)) {
					e.socket->setUDPTarget(currentTarget);
					return e.socket->send(buf, bufSize);
				}
			}
			return false;
		}
	}

	int32_t recv(char* buf, uint32_t bufSize){
		lastUsedSocketIndex = (lastUsedSocketIndex + 1) % routingTable.size();
		uint8_t i = lastUsedSocketIndex;
		do{
			UCRoutingTableEntry& e = routingTable[i];
			int32_t ret = e.socket->recv(buf, bufSize);
			if(ret > 0) {
				lastDatagramAddress = e.socket->getLastDatagramAddress();
				lastUsedSocketIndex = i;
				return ret;
			}
			i = (i + 1) % routingTable.size();
		}while(i!=lastUsedSocketIndex);
		return 0;
	}

};

#endif // uCMultiUDPSocket_H