#ifndef SERIAL_H_INCLUDED
#define SERIAL_H_INCLUDED

/*! C Helper functions for platform independent usage of a serial port*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE SerialPortHandle;
typedef DWORD SerialBaudType;
#else
#include <termios.h>
typedef int SerialPortHandle;
typedef speed_t SerialBaudType;
#endif

struct SerialPort {
	SerialPortHandle handle;
	char const *name;
	#ifdef _WIN32
	bool readBlocking;
	#endif
};

/*! return 0 if successful, otherwise -1, baudRate in baud/s, fills outFlag with the corresponding flag*/
int getBaudFlag(int baudRate, SerialBaudType* outFlag);

/*! returns 0 if successful, */
int openTerm(char const *deviceString, SerialBaudType baud, struct SerialPort *port, bool hwFlowControl, bool readBlocking);

void closeTerm(struct SerialPort const *port);

/*! returns the number of bytes read or -1 in case of an error*/
int recvTermData(struct SerialPort const *port, char* buf, int bufLen);

/*! returns the amount of written bytes, in case of error: -1*/
int sendTermData(struct SerialPort const *port, const char* buf, int bufLen);

/*! Retries the sending of buf until everything is written or an error occurs, returns the amount of written bytes, in case of error the amount of written bytes does not equal bufLen */
int sendWholeTermData(struct SerialPort const *port, const char* buf, int bufLen);

#ifdef __cplusplus
}
#endif

#endif
