#ifndef RS232_H_INCLUDED
#define RS232_H_INCLUDED

#include <string>

struct SerialPort;

class RS232{

	public:

	RS232();

	~RS232();

	//! open serial device, true if successful
	bool open(int baudRate, std::string device, bool hwFlowControl = false, bool readBlocking = false);

	//! send data, true if successful
	bool sendData(const char* buf, int len);

	//! read data and returns the amount of read bytes, -1 in case of error
	int recvData(char* buf, int buflen);

	//! close serial device
	void close();

	private:

	SerialPort* port;

};

#endif
