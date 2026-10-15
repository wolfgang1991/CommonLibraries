#ifndef IUCUDPSLAVESOCKET_H
#define IUCUDPSLAVESOCKET_H

#include "ICommunicationEndpoint.h"
#include "uCIPv4Address.h"

class IUCUDPSocket : public ICommunicationEndpoint{

	public:

	virtual ~IUCUDPSocket() override = default;

	virtual void setUDPTarget(const uCIPv4Address& target) = 0;

	virtual uCIPv4Address getLastDatagramAddress() = 0;

};

#endif // IUCUDPSLAVESOCKET_H