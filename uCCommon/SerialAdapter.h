#ifndef SerialAdapter_H_INCLUDED
#define SerialAdapter_H_INCLUDED

#include "ICommunicationEndpoint.h"

//! Implements ICommunicationEndpoint for Arduino library like serial ports
template <typename TSerial>
class SerialAdapter : public ICommunicationEndpoint{
	
	TSerial& serial;
	
	public:
	
	SerialAdapter(TSerial& serial):serial(serial){
		serial.setTimeout(0);
	}
	
	TSerial& getSerialPort(){
		return serial;
	}
	
	int32_t recv(char* buf, uint32_t bufSize) override{
		int avail = serial.available();
		if(bufSize>0 && avail>0){
			return serial.readBytes(buf, avail>bufSize?bufSize:avail);
		}
		return 0;
	}
	
	//! discard data and do not block e.g. if sending too fast (important for µC reliability)
	//! send buffers of TSerial may need to be adapted
	bool send(const char* buf, uint32_t bufSize) override{
		if(bufSize<=serial.availableForWrite()){
			return serial.write((uint8_t*)buf, bufSize)==bufSize;
		}
		return false;
	}
	
};

#endif
