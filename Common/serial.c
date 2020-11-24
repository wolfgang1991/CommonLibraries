/*
Copyright © 2018-2020 Martin Walch, Wolfgang Wendnagel

This software is provided ‘as-is’, without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <serial.h>

#ifdef _WIN32
/*TODO*/
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#endif

#include <stdio.h>
#include <string.h>

/* baudFlags is the list of system header defined bauds (maybe auto-detect this at some point?)
 * bauds is the list of corresponding values as integers
 * They are expected to correlate and to be sorted in ascending order */
#ifdef __linux__

/* a=(); b=();grep "#define[ ]*B[0-9][0-9]*" /usr/include/x86_64-linux-gnu/bits/termios.h | grep -o B[0-9]* | while read i; do a=(${a[@]} $(echo $i | grep -o B[0-9]*)); done; echo ${a[@]}| sed s/" "/", "/g; echo ${a[@]}| sed -e s/B/""/g -e s/" "/", "/g */
int bauds[] = { 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000 };
SerialBaudType baudFlags[] = { B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000 };

#elif defined __APPLE__

int bauds[] = { 50, 75, 110, 134, 150, 200, 300, 600, 1200, 2400, 4800, 7200, 9600, 14400, 19200, 28800, 38400, 57600, 76800, 115200, 230400 };
SerialBaudType baudFlags[] = { B50, B75, B110, B134, B150, B200, B300, B600, B1200, B2400, B4800, B7200, B9600, B14400, B19200, B28800, B38400, B57600, B76800, B115200, B230400 };

#elif defined _WIN32

int bauds[] = { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000 };
SerialBaudType baudFlags[] = { CBR_110, CBR_300, CBR_600, CBR_1200, CBR_2400, CBR_4800, CBR_9600, CBR_14400, CBR_19200, CBR_38400, CBR_56000, CBR_57600, CBR_115200, CBR_128000, CBR_256000 };

#else

#error no known operating system detected (maybe you can fix me?)

#endif

static int binarySearch(int arr[], int l, int r, int x){
	if(r>=l){
		int mid = l+(r-l)/2;
		if(arr[mid] == x){return mid;}
		if(arr[mid]>x){return binarySearch(arr, l, mid-1, x);}
		return binarySearch(arr, mid+1, r, x);
	}
	return -1; 
}

int getBaudFlag(int baudRate, SerialBaudType* outFlag){
	int idx = binarySearch(bauds, 0, sizeof(bauds)/sizeof(bauds[0])-1, baudRate);
	if(idx>=0){
		*outFlag = baudFlags[idx];
		return 0;
	}
	perror("No matching baud flag found in getBaudFlag.");
	return -1;
}

#ifdef _WIN32
void windows_perror(const char *msg) {
	LPVOID descr;
	DWORD err = GetLastError();

	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
					| FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
			0, (LPTSTR) &descr, 0, NULL);

	fprintf(stderr, "%s: %lu - %s\n", msg, err, (char *)descr);

	LocalFree(descr);
}
#endif

int openTerm(char const *deviceString, SerialBaudType baud, struct SerialPort *port, bool hwFlowControl, bool readBlocking){
#ifdef _WIN32
	HANDLE h;
	DCB settings;
	/* Check if the deviceString is a COM port without any prefix, like e.g.
	 * COM3 or COM55. In that case prepend \\.\ to make it work with port
	 * numbers > 9. This should be safe for all COM ports. See also
	 * https://support.microsoft.com/en-us/help/115831/howto-specify-serial-ports-larger-than-com9 */
	const char *comLiteral = "COM";
	const char *devicePrefix = "\\\\.\\";
	char *normalizedDeviceString;

	if (strncmp(deviceString, comLiteral, strlen(comLiteral)) == 0) {
		normalizedDeviceString = (char*)malloc((strlen(devicePrefix) + strlen(comLiteral)) * sizeof(char));
		if (normalizedDeviceString == NULL) {
			perror("Memory allocation in openTerm()");
			return -1;
		}

		strcpy(normalizedDeviceString, devicePrefix);
		strcpy(normalizedDeviceString + strlen(devicePrefix), deviceString);
	} else {
		normalizedDeviceString = strdup(deviceString);
	}

	/* char * is an LPCSTR and wchar_t * is an LPWSTR. Generic casting would be done with the
	   macro _T(normalizedDeviceString), but we explicitly have a char *, so doing that would look silly */
	h = CreateFile((LPCSTR) normalizedDeviceString, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	free(normalizedDeviceString);

	if (h == INVALID_HANDLE_VALUE) {
		windows_perror("Opening serial port failed");
		return -1;
	}

	GetCommState(h, &settings);
	settings.BaudRate = baud;
	settings.fBinary = TRUE; /* Should be TRUE anyway, but does not hurt */
	settings.fParity = FALSE;
	settings.fOutxCtsFlow = TRUE;
	settings.fOutxDsrFlow = FALSE;
	settings.fDtrControl = DTR_CONTROL_DISABLE;
	settings.fDsrSensitivity = FALSE;
	settings.fTXContinueOnXoff = TRUE;
	settings.fOutX = FALSE;
	settings.fInX = FALSE;
	settings.fNull = FALSE;
	settings.fRtsControl = hwFlowControl?RTS_CONTROL_ENABLE:RTS_CONTROL_DISABLE; /* RTS_CONTROL_TOGGLE instead? Then also configure XoffLim */
	settings.fAbortOnError = FALSE;
	settings.ByteSize = 8;
	settings.Parity = NOPARITY;
	settings.StopBits = ONESTOPBIT; /* equivalent to -cstopb on Unix-like systems*/

	SetCommState(h, &settings);

	port->handle = h;
	port->readBlocking = readBlocking;
	
#else

	struct termios settings;
	int fd;

	/* This is all a mess:
	   * Zeroing out the struct with memset is a bad idea: There may be non-standard fields that must not be all zero
	   * Overwriting (instead of merging) flags is a bad idea: They may carry arbitrary information that we are not aware of
	   * Determining a compatible configuration for capturing is not trivial
	 */

	fd = open(deviceString, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("Opening device");
		return -1;
	}

	tcgetattr(fd, &settings);

	cfsetospeed(&settings, baud);
	cfsetispeed(&settings, baud);

	settings.c_iflag &= ~(IGNBRK | IGNPAR | ICRNL | IXON | IXOFF); /* Just pass as is */
	settings.c_oflag &= ~(OPOST | ONLCR);
	settings.c_cflag |= CS8 | CREAD | CLOCAL;
	settings.c_cflag &= ~(CSTOPB);
	settings.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHONL);
	settings.c_cflag &= ~(PARENB | PARODD);
	if(hwFlowControl){
		settings.c_cflag |= CRTSCTS;
	}else{
		settings.c_cflag &= ~CRTSCTS;
	}
	settings.c_cc[VMIN] = readBlocking?1:0;
	settings.c_cc[VTIME] = 0;

	tcflush(fd, TCIFLUSH); /* flush input, preserve output */
	if (tcsetattr(fd, TCSAFLUSH, &settings)) {
		perror("Configuring serial line");
		close(fd);
		return -1;
	}

	port->handle = fd;

#endif

	port->name = deviceString;
	return 0;
}

void closeTerm(struct SerialPort const *port) {
#ifdef _WIN32
	CloseHandle(port->handle);
#else
	close(port->handle);
#endif
}

static inline int serial_min(int a, int b){return a<b?a:b;}

int recvTermData(struct SerialPort const *port, char* buf, int bufLen){
	#ifdef _WIN32
	DWORD errors;
	COMSTAT stat;
	DWORD bytesRead = 0;
	if (!ClearCommError(port->handle, &errors, &stat)) {
		windows_perror("during ClearCommError()");
		fprintf(stderr, "Occurred on access to %s\n", port->name);
		return -1;
	}
	if (errors != 0x0) {
		fprintf(stderr, "Non-zero error word from ClearCommError(): %lX\n", errors);
		return -1;
	}
	if(stat.cbInQue>0 || port->readBlocking){
		if (!ReadFile(port->handle, buf, port->readBlocking?bufLen:serial_min(stat.cbInQue,bufLen), &bytesRead, NULL)) {
			windows_perror("during ReadFile()");
			fprintf(stderr, "Occurred on read access to %s\n", port->name);
			return -1;
		}
	}
	#else
	ssize_t bytesRead = read(port->handle, buf, bufLen);
	if (bytesRead == -1) {
		perror("during read(int, void *, size_t)");
		fprintf(stderr, "Occurred on read access to %s\n", port->name);
		return -1;
	}
	#endif
	return bytesRead;
}

int sendTermData(struct SerialPort const *port, const char* buf, int bufLen){
	#ifdef _WIN32
	DWORD written;
	if (!WriteFile(port->handle, buf, bufLen, &written, NULL)) {
		windows_perror("during WriteFile()");
		fprintf(stderr, "Occurred on write access to %s with data:\n", port->name);
		fwrite(buf, 1, bufLen, stderr);
		fprintf(stderr, "\n");
		return -1;
	}
	#else
	ssize_t written = write(port->handle, buf, bufLen);
	if(written==-1){
		perror("during write(int, void *, size_t)");
		fprintf(stderr, "Occurred on write access to %s with data:\n", port->name);
		fwrite(buf, 1, bufLen, stderr);
		fprintf(stderr, "\n");
		return -1;
	}
	#endif
	return written;
}

int sendWholeTermData(struct SerialPort const *port, const char* buf, int bufLen){
	int writtenTotal = 0;
	while(writtenTotal<bufLen){
		int written = sendTermData(port, &buf[writtenTotal], bufLen-writtenTotal);
		if(written==-1){
			return writtenTotal;
		}else{
			writtenTotal += written;
			if(writtenTotal<bufLen){/* yield */
				#if _WIN32
				Sleep(0);
				#else
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = 1;
				nanosleep(&ts, NULL);
				#endif
			}
		}
	}
	return writtenTotal;
}

#ifdef __cplusplus
}
#endif
