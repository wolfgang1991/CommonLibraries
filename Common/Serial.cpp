#include <Serial.h>

#include <serial.h>
#include <platforms.h>

#if defined(LINUX_PLATFORM) || defined(ANDROID_PLATFORM)
#include <dirent.h>
#endif

#include <cstring>

Serial::Serial(){
	port = NULL;
}

Serial::~Serial(){
	close();
}

bool Serial::open(int baudRate, std::string device, bool hwFlowControl, bool readBlocking){
	close();
	SerialBaudType baudFlag;
	int res = getBaudFlag(baudRate, &baudFlag);
	if(res==0){
		port = new SerialPort();
		res = openTerm(device.c_str(), baudFlag, port, hwFlowControl, readBlocking);
		if(res!=0){delete port; port = NULL;}
	}
	return res==0;
}

bool Serial::send(const char* buf, uint32_t len){
	if(port){
		int res = sendWholeTermData(port, buf, len);
		return res==(int)len;
	}
	return false;
}

int32_t Serial::recv(char* buf, uint32_t buflen){
	if(port){
		return recvTermData(port, buf, buflen);
	}
	return -1;
}

void Serial::close(){
	if(port){
		closeTerm(port);
		delete port;
		port = NULL;
	}
}

std::list<std::string> findSerialPorts(){
	std::list<std::string> res;
	#if defined(LINUX_PLATFORM) || defined(ANDROID_PLATFORM)
	struct dirent* entry = NULL;
    DIR* dp = opendir("/dev");
    if(dp != NULL) {
		while((entry = readdir(dp))){
			if(strstr(entry->d_name, "ttyACM")!=NULL || strstr(entry->d_name, "ttyUSB")!=NULL || strstr(entry->d_name, "ttyS")!=NULL){
				res.push_back(std::string("/dev/")+entry->d_name);
			}
		}
    }
    closedir(dp);
	#else
	#warning "findSerialPorts not implemented for this platform"
	#endif
	return res;
}
