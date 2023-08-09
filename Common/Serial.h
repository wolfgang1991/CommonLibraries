#ifndef Serial_H_INCLUDED
#define Serial_H_INCLUDED

#include "ICommunicationEndpoint.h"

#include <string>
#include <list>

struct SerialPort;

class Serial : public ICommunicationEndpoint{

	//auto reconnnect stuff
	bool reconnectEnabled;
	int baudRate;
	std::list<std::string> deviceNodes;
	std::list<std::string>::iterator currentDeviceNode;
	double receiveTimeout, lastReceiveTime;
	bool hwFlowControl, readBlocking;
	
	bool openWithFields();

	public:

	Serial();

	~Serial();

	//! open serial device, true if successful
	//! clears reconnect settings
	bool open(int baudRate, std::string device, bool hwFlowControl = false, bool readBlocking = false);
	
	//! alternativeDevices: list of alternative device nodes (useful in combination with udev rules to create special device node names for uid/vid pairs)
	//! open needs to be called before
	void setAutoReconnect(bool enabled, double receiveTimeout, const std::list<std::string>& alternativeDevices);

	//! send data, true if successful
	bool send(const char* buf, uint32_t len);

	//! read data and returns the amount of read bytes, -1 in case of error
	int32_t recv(char* buf, uint32_t buflen);

	//! close serial device
	void close();

	private:

	SerialPort* port;

};

//! usbOnly: only return serial ports connected via USB
//! returns strings usable for device parameter in RS232 class
std::list<std::string> findSerialPorts();

#endif
