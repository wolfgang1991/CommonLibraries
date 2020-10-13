#ifndef RS232_H_INCLUDED
#define RS232_H_INCLUDED

#include <ICommunicationEndpoint.h>

#include <string>

struct SerialPort;

class RS232 : public ICommunicationEndpoint{

	public:

	RS232();

	~RS232();

	//! open serial device, true if successful
	bool open(int baudRate, std::string device, bool hwFlowControl = false, bool readBlocking = false);

	//! send data, true if successful
	bool send(const char* buf, uint32_t len);

	//! read data and returns the amount of read bytes, -1 in case of error
	int32_t recv(char* buf, uint32_t buflen);

	//! close serial device
	void close();

	private:

	SerialPort* port;

};

#endif
