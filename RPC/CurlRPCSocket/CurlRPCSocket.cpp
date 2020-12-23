#include "CurlRPCSocket.h"

#include <Threading.h>
#include <timing.h>

#include <curl/curl.h>

#include <sstream>
#include <cstring>

#define CURL_RETRY_PERIOD 600000 // ms = 10 min

class CurlRPCSocketPrivate{
	
	public:
	
	struct Data{
		char* data;
		uint32_t size;
	};
	
	//curl thread only:
	std::stringstream ss;
	bool sessionIDRead;//in reply
	std::string sessionID;
	std::string url;
	std::function<bool(char)> isFullRequestDetected;
	CurlRPCSocket::Method method;
	bool verifySSLHost;
	bool verifySSLPeer;
	
	//exchange:
	Mutex m;
	bool mustExit;
	std::list<Data> toServer;
	std::list<Data> fromServer;
	uint32_t readOffset;
	
	//main thread only:
	Thread t;
	bool threadCreateSuccess;
	uint32_t maxPendingBytes;
	
	static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata){
		if(nmemb>0){
			size_t actualSize = size*nmemb;
			CurlRPCSocketPrivate* p = (CurlRPCSocketPrivate*)userdata;
			if(!p->sessionIDRead){
				for(size_t i=0; i<actualSize; i++){
					char c = ptr[i];
					p->ss << c;
					if(c==';'){
						p->sessionIDRead = true;
						p->sessionID = p->ss.str();
						i++;
						if(i<actualSize){ptr = &ptr[i];}
						actualSize -= i;
					}
				}
			}
			if(p->sessionIDRead && actualSize>0){
				Data rcv{new char[actualSize], (uint32_t)actualSize};
				memcpy(rcv.data, ptr, actualSize);
				lockMutex(p->m);
				p->fromServer.push_back(rcv);
				unlockMutex(p->m);
			}
		}
		return size*nmemb;
	}
	
	static void* curlMain(void* data){
		CurlRPCSocketPrivate* p = (CurlRPCSocketPrivate*)data;
		CURL* curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);//for thread safety
		curl_easy_setopt(curl, CURLOPT_NOPROXY, "localhost,127.0.0.1");//ipv6 doens't work in this list
		curl_easy_setopt(curl, CURLOPT_URL, p->url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, p->verifySSLPeer?1L:0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, p->verifySSLHost?2L:0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		//TODO set timeout / speed timeout for upload / download 
		std::list<Data> toSend;
		std::list<Data>::iterator lastParsed = toSend.end();
		uint32_t totalSize = p->sessionID.size();//start with sessionID (; separated) content follows
		bool running = true;
		while(running){
			//Exchange
			lockMutex(p->m);
			running = !p->mustExit;
			toSend.splice(toSend.end(), p->toServer);
			unlockMutex(p->m);
			if(running && !toSend.empty()){
				//Process data to send to find balanced end
				std::list<Data>::iterator start;
				if(lastParsed==toSend.end()){
					start = toSend.begin();
				}else{
					start = lastParsed;
					start++;
				}
				std::list<Data>::iterator balanced = toSend.end();
				for(auto it = start; it != toSend.end(); ++it){
					Data& d = *it;
					bool isFull = false;
					for(uint32_t i=0; i<d.size; i++){
						isFull = p->isFullRequestDetected(d.data[i]);
					}
					totalSize += d.size;
					if(isFull){
						balanced = it;
					}
				}
				//put data together and perfom curl stuff
				if(balanced!=toSend.end()){
					balanced++;
					char* totalData = new char[totalSize];
					memcpy(totalData, p->sessionID.c_str(), p->sessionID.size());
					uint32_t offset = p->sessionID.size();
					auto it = toSend.begin();
					while(it!=balanced){
						memcpy(&totalData[offset], it->data, it->size);
						offset += it->size;
						delete[] it->data;
						it = toSend.erase(it);
					}
					p->sessionIDRead = false;
					p->ss.str("");
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, totalData);
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)totalSize);
					CURLcode result = CURLE_FAILED_INIT;//just for init != CURLE_OK
					while(running && result!=CURLE_OK){
						result = curl_easy_perform(curl);
						if(result!=CURLE_OK){
							lockMutex(p->m);
							running = !p->mustExit;
							unlockMutex(p->m);
							delay(CURL_RETRY_PERIOD);
						}
					}
					totalSize = p->sessionID.size();
					delete[] totalData;
				}
				//remember where to continue parsing for balanced end
				lastParsed = toSend.end();
				if(!toSend.empty()){
					--lastParsed;
				}
			}
			delay(1);
		}
		curl_easy_cleanup(curl);
		delete p;
		return NULL;
	}
	
	CurlRPCSocketPrivate(const std::string& url, bool verifySSLHost, bool verifySSLPeer, const std::function<bool(char)>& isFullRequestDetected, uint32_t maxPendingBytes, CurlRPCSocket::Method method):
		sessionIDRead(false),sessionID(";"),url(url),isFullRequestDetected(isFullRequestDetected),method(method),verifySSLHost(verifySSLHost),verifySSLPeer(verifySSLPeer),mustExit(false),readOffset(0),maxPendingBytes(maxPendingBytes){
		initMutex(m);
		threadCreateSuccess = createThread(t, curlMain, this, false);
	}
	
	~CurlRPCSocketPrivate(){
		deleteMutex(m);
		for(Data& d : toServer){
			delete[] d.data;
		}
		for(Data& d : fromServer){
			delete[] d.data;
		}
	}
	
	bool hlp_send(const char* buf, uint32_t bufSize){
		if(bufSize>0){
			uint32_t sumSendSize = 0;
			lockMutex(m);
			for(auto& d : toServer){
				sumSendSize += d.size;
			}
			unlockMutex(m);
			if(sumSendSize<maxPendingBytes){
				Data snd{new char[bufSize], bufSize};
				memcpy(snd.data, buf, bufSize);
				lockMutex(m);
				toServer.push_back(snd);
				unlockMutex(m);
				return true;
			}else{
				return false;
			}
		}else{
			return true;
		}
	}
	
	uint32_t hlp_recv(char* buf, uint32_t bufSize){
		uint32_t sumCopied = 0;
		if(bufSize>0){
			lockMutex(m);
			while(!fromServer.empty() && bufSize>0){
				Data& d = fromServer.front();
				uint32_t avail = d.size-readOffset;
				unlockMutex(m);
				uint32_t toCopy = avail>bufSize?bufSize:avail;
				memcpy(buf, &d.data[readOffset], toCopy);
				bufSize -= toCopy;
				if(bufSize>0){buf = &buf[toCopy];}
				lockMutex(m);
				sumCopied += toCopy;
				readOffset += toCopy;
				if(readOffset==d.size){
					delete[] d.data;
					fromServer.pop_front();
					readOffset = 0;
				}
			}
			unlockMutex(m);
		}
		return sumCopied;
	}
	
};

CurlRPCSocket::CurlRPCSocket(const std::string& url, bool verifySSLHost, bool verifySSLPeer, const std::function<bool(char)>& isFullRequestDetected, uint32_t maxPendingBytes, Method method){
	prv = new CurlRPCSocketPrivate(url, verifySSLHost, verifySSLPeer, isFullRequestDetected, maxPendingBytes, method);
}
	
CurlRPCSocket::~CurlRPCSocket(){
	if(prv->threadCreateSuccess){
		lockMutex(prv->m);
		prv->mustExit = true;
		unlockMutex(prv->m);
	}else{
		delete prv;
	}
}
	
uint32_t CurlRPCSocket::getAvailableBytes() const{
	uint32_t sum = 0;
	lockMutex(prv->m);
	for(auto& d : prv->fromServer){
		sum += d.size;
	}
	sum -= prv->readOffset;
	unlockMutex(prv->m);
	return sum;
}

uint32_t CurlRPCSocket::recv(char* buf, uint32_t bufSize, bool readBlocking){
	uint32_t res = prv->hlp_recv(buf, bufSize);
	while(readBlocking && res==0){
		delay(10);
		res = prv->hlp_recv(buf, bufSize);
	}
	return res;
}

bool CurlRPCSocket::send(const char* buf, uint32_t bufSize){
	return prv->hlp_send(buf, bufSize);
}

struct JSONFullRequestDetector{
	int32_t squareBracketCount;
	int32_t curlyBracketCount;
	int state;//0: normal, 1: inside string, 2: inside escape
};

std::function<bool(char)> createJSONRPCFullRequestDetectionFunction(){
	JSONFullRequestDetector d{0,0,0};
	return [d](char c)mutable{
		switch(d.state){
			case 0:{
				switch(c){
					case '{':{
						d.curlyBracketCount++;
						break;
					}case '}':{
						d.curlyBracketCount--;
						break;
					}case '[':{
						d.squareBracketCount++;
						break;
					}case ']':{
						d.squareBracketCount--;
						break;
					}case '\"':{
						d.state = 1;
						break;
					}
				}
				break;
			}case 1:{
				switch(c){
					case '\"':{
						d.state = 0;
						break;
					}case '\\':{
						d.state = 2;
						break;
					}
				}
				break;
			}case 2:{
				d.state = 1;
				break;
			}
		}
		return d.state==0 && d.squareBracketCount==0 && d.curlyBracketCount==0;
	};
}
