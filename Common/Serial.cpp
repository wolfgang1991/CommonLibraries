#include <Serial.h>

#include <cserial.h>
#include <platforms.h>
#include <timing.h>

#if defined(LINUX_PLATFORM) || defined(ANDROID_PLATFORM)
#include <dirent.h>
#endif

#include <cstring>

Serial::Serial(){
	reconnectEnabled = false;
	port = NULL;
}

Serial::~Serial(){
	close();
}

bool Serial::openWithFields(){
	SerialBaudType baudFlag;
	int res = getBaudFlag(baudRate, &baudFlag);
	if(res==0){
		port = new SerialPort();
		res = openTerm(currentDeviceNode->c_str(), baudFlag, port, hwFlowControl, readBlocking);
		if(res!=0){delete port; port = NULL;}
	}
	return res==0;
}

bool Serial::open(int baudRate, std::string device, bool hwFlowControl, bool readBlocking){
	close();
	reconnectEnabled = false;
	this->baudRate = baudRate;
	deviceNodes.clear();
	deviceNodes.push_back(device);
	currentDeviceNode = deviceNodes.begin();
	this->hwFlowControl = hwFlowControl;
	this->readBlocking = readBlocking;
	return openWithFields();
}

void Serial::setAutoReconnect(bool enabled, double receiveTimeout, const std::list<std::string>& alternativeDevices){
	for(const std::string& s : alternativeDevices){
		deviceNodes.emplace_back(s);
	}
	reconnectEnabled = enabled && !deviceNodes.empty();
	this->receiveTimeout = receiveTimeout;
	lastReceiveTime = getSecs();
}

bool Serial::send(const char* buf, uint32_t len){
	if(port){
		int res = sendWholeTermData(port, buf, len);
		return res==(int)len;
	}
	return false;
}

int32_t Serial::recv(char* buf, uint32_t buflen){
	int32_t res = -1;
	if(port){
		res = recvTermData(port, buf, buflen);
	}
	if(reconnectEnabled){//deviceNodes not empty if reconnectEnabled
		if(res<=0){
			if(getSecs()-lastReceiveTime>=receiveTimeout){
				std::list<std::string>::iterator startIt = currentDeviceNode;
				bool success = false;
				do{
					currentDeviceNode++;
					if(currentDeviceNode==deviceNodes.end()){currentDeviceNode = deviceNodes.begin();}
					success = openWithFields();
				}while(currentDeviceNode!=startIt && !success);
				lastReceiveTime = getSecs();
				if(port){
					res = recvTermData(port, buf, buflen);
				}
			}
		}else{
			lastReceiveTime = getSecs();
		}
	}
	return res;
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
