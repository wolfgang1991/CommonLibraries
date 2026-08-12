#include "RTSPClient.h"

#include <StringHelpers.h>
#include <SimpleSockets.h>
#include <timing.h>

#include <unordered_map>
#include <sstream>
#include <iostream>
#include <memory>

#define TCP_CONNECT_TIMEOUT 1000 //ms
#define RTSP_RECV_BUF_SIZE 1024

class RTSPClientPrivate{

	public:
	
	bool useMulticast;
	std::function<void(const IIPAddress&, bool)> OnPlay;
	
	std::shared_ptr<IIPAddress> address;
	
	int state;//0: not connected, 1: connecting (mjpg), 2: options sent (mjpg), 3: describe sent (mjpg), 4: setup sent (mjpg), 5: play sent (mjpg), 6: playing  (mjpg)
	uint32_t cseq;
	
	int action;//0: disable, 1: play
	
	std::string url;
	uint16_t rtpPort;
	double heartBeatPeriod;
	
	ASocket* s;
	
	std::string hostname;
	uint16_t rtspPort;
	
	std::stringstream ss;
	char buf[RTSP_RECV_BUF_SIZE];
	
	//last header
	std::string firstLine;
	std::unordered_map<std::string, std::string> fields;
	
	std::string sessionID;
	
	double lastHeartbeatTime, lastReceiveTime;
	
	void fillSameFieldsAndSend(std::stringstream& ss){
		ss << "CSeq: " << cseq << "\r\n";
		ss << "User-Agent: MissionServer\r\n\r\n";
		cseq++;
		std::string toSend = ss.str();
		s->send(toSend.c_str(), toSend.size());
		std::cout << "send (" << hostname << ") : \"" << toSend << "\"\n" << std::flush;
	}
	
	void sendOptions(){
		std::stringstream ss;
		ss << "OPTIONS " << url << " RTSP/1.0\r\n";
		fillSameFieldsAndSend(ss);
	}
	
	void sendDescribe(){
		std::stringstream ss;
		ss << "DESCRIBE " << url << " RTSP/1.0\r\n";
		ss << "Accept: application/sdp\r\n";
		fillSameFieldsAndSend(ss);
	}
	
	void sendUnicastSetup(const std::string& controlString){ // old LWIR: "trackID=0" new LWIR: track_video
		std::stringstream ss;
		//ss << "SETUP " << url << "/trackID=0 RTSP/1.0\r\n";
		ss << "SETUP " << url << "/" << controlString << " RTSP/1.0\r\n";
		ss << "Transport: RTP/AVP/UDP;unicast;client_port=" << rtpPort << "-" << (rtpPort+1) << "\r\n";
		fillSameFieldsAndSend(ss);
	}
	
	void sendMulticastSetup(const std::string& controlString){
		std::stringstream ss;
		//ss << "SETUP " << url << "/trackID=0 RTSP/1.0\r\n";
		ss << "SETUP " << url << "/" << controlString << " RTSP/1.0\r\n";
		ss << "Transport: RTP/AVP;multicast\r\n";
		fillSameFieldsAndSend(ss);
	}
	
	void sendPlay(){
		std::stringstream ss;
		ss << "PLAY " << url << "/ RTSP/1.0\r\n";
		ss << "Range: npt=0.000-\r\n";
		ss << "Session: " << sessionID << "\r\n";
		fillSameFieldsAndSend(ss);
	}
	
	void sendTeardown(){
		std::stringstream ss;
		ss << "TEARDOWN " << url << " RTSP/1.0\r\n";
		ss << "Session: " << sessionID << "\r\n";
		fillSameFieldsAndSend(ss);
	}
	
	//! -1 if unsuccessful, otherwise header length / payload offset
	int parseHeader(const std::string& response){
		firstLine.clear();
		fields.clear();
		std::stringstream firstLineSS;
		std::stringstream keySS;
		std::stringstream valueSS;
		int state = 0;//0 first line before CR, 1 first line before LF, 2 key start, 3 parsing key, 4 value start, 5 parsing value, 6 CR in value read, 7 empty key CR read
		for(int i=0; i<(int)response.size(); i++){
			char c = response[i];
			if(state==0){
				if(c=='\r'){
					state = 1;
				}else{
					firstLineSS << c;
				}
			}else if(state==1){
				if(c=='\n'){
					firstLine = firstLineSS.str();
					state = 2;//keySS and valueSS empty already
				}else{
					firstLineSS << '\r' << c;
					state = 0;
				}
			}else if(state==2){
				if(c==':'){//empty key
					state = 4;
				}else if(c=='\r'){
					state = 7;
				}else{
					keySS << c;
					state = 3;
				}
			}else if(state==3){
				if(c==':'){
					state = 4;
				}else{
					keySS << c;
				}
			}else if(state==4){
				if(c=='\r'){//empty value
					state = 6;
				}else{
					if(c!=' '){valueSS << c;}
					state = 5;
				}
			}else if(state==5){
				if(c=='\r'){
					state = 6;
				}else{
					valueSS << c;
				}
			}else if(state==6){
				if(c=='\n'){
					state = 2;
					fields[keySS.str()] = valueSS.str();
					keySS.str("");
					valueSS.str("");
				}else{
					valueSS << '\r' << c;
					state = 5;
				}
			}else if(state==7){
				if(c=='\n'){
					return i+1;//state = 8;
				}else{
					state = 3;
					keySS << '\r' << c;
				}
			}
		}
		return -1;
	}
	
	void reset(){
		delete s;
		s = NULL;
		state = 0;
		action = 0;
	}
	
	RTSPClientPrivate(const std::string& url, uint16_t rtpPort, double heartBeatPeriod):state(0),cseq(1),action(0),url(url),rtpPort(rtpPort),heartBeatPeriod(heartBeatPeriod),s(NULL){
		if(url.size()>7){//"rtsp://"
			if(convertStringToUpper(url.substr(0,7))=="RTSP://"){
				auto colonPos = url.find(':', 7);
				if(colonPos<url.size()){
					hostname = url.substr(7, colonPos-7);
					auto portStartPos = colonPos+1;
					auto i = portStartPos;
					for(; i<url.size(); i++){
						char c = url[i];
						if(c<'0' || c>'9'){break;}
					}
					rtspPort = convertStringTo<uint16_t>(url.substr(portStartPos, i-portStartPos));
					//std::cout << "hostname: " << hostname << " rtspPort: " << rtspPort << std::endl;
				}
			}
		}
	}
	
	~RTSPClientPrivate(){
		delete s;
	}
	
	void update(){
		if(state==0){
			if(action==1){
				std::cout << "Connecting to: " << url << std::endl;
				if(!hostname.empty()){
					s = createSocketForHostName(true, hostname, rtspPort, TCP_CONNECT_TIMEOUT);
				}
				lastReceiveTime = getSecs();
				if(s){
					state = 1;
				}else{
					std::cerr << "Connection unsuccessful." << std::endl;
					action = 0;
				}
			}
		}else{
			double t = getSecs();
			//read possible response from server
			uint32_t received = s->recv(buf, RTSP_RECV_BUF_SIZE);
			if(received>0){
				lastReceiveTime = t;
			}else if(t-lastReceiveTime>2*heartBeatPeriod){//timeout
				std::cout << "RTSP timeout: " << url << std::endl;
				reset();
				return;
			}
			while(received>0){
				std::cout << "received (" << hostname << "): \"";
				std::cout.write(buf, received); std::cout << "\"\n" << std::flush;
				ss.write(buf, received);
				received = s->recv(buf, RTSP_RECV_BUF_SIZE);
			}
			//parse header
			std::string incoming = ss.str();
			int headerSize = parseHeader(incoming);
			bool newResponse = false;
			if(headerSize>0){
				uint32_t requiredLength = convertStringTo<uint32_t>(fields["Content-Length"]) + headerSize;//empty == 0
				newResponse = incoming.size()==requiredLength;
				//std::cout << "header parsed incoming.size(): " << incoming.size() << " requiredLength: " << requiredLength << std::endl;
				if(requiredLength<incoming.size()){
					std::cerr << "Excess bytes in response:\n" << incoming << std::endl;
				}else if(newResponse){
					ss.str("");
				}
			}
			if(newResponse && firstLine!="RTSP/1.0 200 OK"){//Error in first line / return code
				std::cerr << "Error returned from RTSP: " << firstLine << std::endl;
				reset();
				return;
			}
			//Communication state dependent //0: not connected, 1: connecting (mjpg), 2: options sent (mjpg), 3: describe sent (mjpg), 4: setup sent (mjpg), 5: play sent (mjpg), 6: playing (mjpg), 7: playing (heartbeat sent)
			if(state==1){
				sendOptions();
				state = 2;
			}else if(state==2){
				if(newResponse){
					std::cout << "Options result: " << fields["Public"] << std::endl;
					sendDescribe();
					state = 3;
				}
			}else if(state==3){
				if(newResponse){
					if(incoming.find("JPEG")==std::string::npos){//TODO better checking
						std::cerr << "No Motion JPEG encoding is used." << std::endl;
						reset();
					}else{
						std::string controlString;
						size_t controlPos = 0;
						while(controlPos<incoming.size() && (controlString.empty() || controlString=="*")){//some cameras return a=* which is useless
							controlPos = incoming.find("a=control:", controlPos);
							if(controlPos==std::string::npos){
								std::cerr << "ERROR: No control string found in describe response, using track_video." << std::endl;
								controlString = "track_video";
							}else{
								controlPos += 10;//length of "a=control:"
								size_t controlEnd = incoming.find("\r\n", controlPos);
								if(controlEnd==std::string::npos){
									controlEnd = incoming.size();
								}
								controlString = incoming.substr(controlPos, controlEnd-controlPos);
								std::cout << "Using control string: " << controlString << std::endl;
								if(isPrefixEqual(controlString, "rtsp://")){//full url
									std::cerr << "WARNING: Full URL in control string not supported, using only the part after the last /." << std::endl;
									size_t lastSlash = controlString.rfind('/');//must exist because of "rtsp://"
									controlString = controlString.substr(lastSlash+1, std::string::npos);
								}
								controlPos = controlEnd;
							}
						}
						if(useMulticast){
							sendMulticastSetup(controlString);
						}else{
							sendUnicastSetup(controlString);
						}
						state = 4;
					}
				}
			}else if(state==4){
				if(newResponse){
					std::list<std::string> sessionFields = parseSeparatedString(fields["Session"], ';');
					std::list<std::string> transportFields = parseSeparatedString(fields["Transport"], ';');
					if(sessionFields.empty()){
						std::cerr << "Error: No Session ID in Session value: " << fields["Session"] << std::endl;
						reset();
					}else{
						sessionID = sessionFields.front();
						if(useMulticast){
							std::string ip, port;
							for(const std::string& s : transportFields){
								if(isPrefixEqual(s, "destination=") && s.size()>12){
									ip = s.substr(12, std::string::npos);
								}else if(isPrefixEqual(s, "port=") && s.size()>5){
									size_t dashPos = s.find('-', 5);
									if(dashPos==std::string::npos){
										port = s.substr(5, std::string::npos);
									}else{//first port is rtp, second (the following is rtcp)
										port = s.substr(5, dashPos-5);
									}
								}
							}
							if(ip.empty() || port.empty()){
								std::cerr << "IP address or port empty in rtsp multicast reply. IP address: " << ip << " port: " << port << std::endl;
								reset();
								return;
							}else{
								if(ip.find(':')==std::string::npos){//IPv4 address without :
									address = std::shared_ptr<IIPAddress>(new IPv4Address(ip, convertStringTo<uint16_t>(port)));
								}else{
									address = std::shared_ptr<IIPAddress>(new IPv6Address(ip, convertStringTo<uint16_t>(port)));
								}
							}
						}else{
							address = std::shared_ptr<IIPAddress>(new IPv4Address("127.0.0.1", rtpPort));
						}
						sendPlay();
						state = 5;
					}
				}
			}else if(state==5){
				if(newResponse){
					OnPlay(*address, useMulticast);
					lastHeartbeatTime = t;
					state = 6;
				}
			}else if(state==6){
				if(t-lastHeartbeatTime>heartBeatPeriod){
					lastHeartbeatTime = t;
					sendOptions();
					state = 7;
				}else if(action==0){
					sendTeardown();
					reset();
				}
			}else if(state==7){
				if(newResponse){
					state = 6;
				}
			}
		}
	}
	
};


RTSPClient::RTSPClient(const std::string& url, uint16_t rtpPort, double heartBeatPeriod){
	p = new RTSPClientPrivate(url, rtpPort, heartBeatPeriod);
}
	
RTSPClient::~RTSPClient(){
	stop();
	update();
	delete p;
}

RTSPClient::State RTSPClient::getState() const{
	return p->state==0?IDLE:(p->state>=6?PLAYING:SETUP);
}

void RTSPClient::update(){
	p->update();
}

void RTSPClient::play(const std::function<void(const IIPAddress&, bool)>& OnPlay, bool useMulticast){
	p->OnPlay = OnPlay;
	p->useMulticast = useMulticast;
	p->action = 1;
}
	
void RTSPClient::stop(){
	p->action = 0;
}
