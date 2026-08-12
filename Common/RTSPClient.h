#ifndef RTSPClient_H_
#define RTSPClient_H_

#include <string>
#include <cstdint>
#include <functional>

class RTSPClientPrivate;
class IIPAddress;

//! wraps rtsp requests into nice methods, it may be blocking some time in case of connection and send
class RTSPClient{

	RTSPClientPrivate* p;
	
	public:
	
	enum State{IDLE, SETUP, PLAYING, STATE_COUNT};
	
	//! url e.g. e.g. rtsp://192.168.1.108:554, creates a new tcp connection; rtpPort: udp port @ client, rtcp port will be rtpPort+1 (rtpPort is not used in case of multicast / server defines ports in case of multicast)
	RTSPClient(const std::string& url, uint16_t rtpPort, double heartBeatPeriod = 10.0);
	
	virtual ~RTSPClient();
	
	State getState() const;
	
	void update();
	
	//! currently checks for MJPG and fails if not used, since other CommonLibraries implementations only support MJPG so far
	//! address of the callback contains a localhost address if used with unicast or the multicast address if used with multicast
	void play(const std::function<void(const IIPAddress& address, bool useMulticast)>& OnPlay, bool useMulticast);
	
	void stop();
	
};

#endif
