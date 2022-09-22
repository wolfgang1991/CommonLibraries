#ifndef NMEAPrint_H_INCLUDED
#define NMEAPrint_H_INCLUDED

#include "uCTypeTraits.h"

#include <inttypes.h>

//! Prints parts of NMEA strings while updating the checksum
template <typename TSerial>
class NMEAPrint{
	
	private:
	
	TSerial& serial;
	char checksum;
	
	char buf[15];//fits for all integers
	
	public:
	
	NMEAPrint(TSerial& serial):serial(serial),checksum(0){}
	
	void beginSentence(){
		checksum = 0;
		serial.print('$');
	}
	
	void printPart(const char* part){
		serial.print(part);
		while(*part){
        	checksum ^= *part++;
        }
	}
	
	void printPart(char c){
		serial.print(c);
		checksum ^= c;
	}
	
	template <typename TInteger, typename ucstd::enable_if<ucstd::is_same<TInteger,int8_t>::value || ucstd::is_same<TInteger,int16_t>::value || ucstd::is_same<TInteger, int32_t>::value, int>::type = 0>
	void printPart(TInteger part){
		sprintf(buf, "%" PRId32, (int32_t)part);
		printPart(buf);
	}
	
	template <typename TInteger, typename ucstd::enable_if<ucstd::is_same<TInteger,uint8_t>::value || ucstd::is_same<TInteger,uint16_t>::value || ucstd::is_same<TInteger,uint32_t>::value, int>::type = 0>
	void printPart(TInteger part){
		sprintf(buf, "%" PRIu32, (uint32_t)part);
		printPart(buf);
	}
	
	template <typename TFloat, typename ucstd::enable_if<ucstd::is_same<TFloat,float>::value || ucstd::is_same<TFloat,double>::value>::type = 0>
	void printPart(TFloat part){
		sprintf(buf, "%f", part);
		printPart(buf);
	}
	
	void endSentence(){
		serial.print('*');
		if(checksum<16){
			serial.print('0');//leading zero
		}
		serial.print((int)checksum, HEX);
		serial.print("\r\n");
	}
	
};

#endif
