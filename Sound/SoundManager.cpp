#include "SoundManager.h"
#include "ISoundDriver.h"
#include "AlsaSoundDriver.h"

#include <Threading.h>
#include <platforms.h>
#include <timing.h>

#include <map>
#include <cassert>
#include <iostream>

class SoundManagerPrivate{
	
	public:
	
	ISoundDriver* driver;
	double maxBufferTime;
	
	struct ThreadContext{
		Thread t;
		Mutex m;
		double maxBufferTime;
		ISoundDriver* driver;
		ISoundSource* source;
		bool isPause;
		bool mustExit;
	};
	
	static void* threadMain(void* data){
		ThreadContext* tc = (ThreadContext*)data;
		IPCMPlaybackContext* c = tc->driver->createPlaybackContext(tc->source);
		double maxBufferTime = tc->maxBufferTime;
		uint32_t bytesPerSecond = tc->source->getChannelCount()*tc->source->getSampleRate();
		uint32_t maxBytesUpdate = bytesPerSecond*maxBufferTime;
		double secondsPerFrame = 1.0/tc->source->getSampleRate();
//		double bufferedUntil = getSecs();
		bool running = true;
		bool isPause = false;
		while(running){
			lockMutex(tc->m);
			isPause = tc->isPause;
			running = !tc->mustExit;
			unlockMutex(tc->m);
			uint32_t bytesUpdated = 0;
//			double t = getSecs();
//			if(t>=bufferedUntil-maxBufferTime){
//				bytesUpdated = c->update(isPause, maxBytesUpdate);
//				bufferedUntil = bufferedUntil+((double)bytesUpdated)/bytesPerSecond;
//			}
			double bufferTime = c->getDelayFrameCount()*secondsPerFrame;
			std::cout << "bufferTime: " << bufferTime << std::endl;
			if(bufferTime<maxBufferTime){
				bytesUpdated = c->update(isPause, maxBytesUpdate);
			}
			if(bytesUpdated==0){delay(2);}//avoid busy wait if no update
		}
		delete c;
		return NULL;
	}
	
	std::map<ISoundSource*, ThreadContext> c;
	
	void stop(ThreadContext* tc){
		lockMutex(tc->m);
		tc->mustExit = true;
		unlockMutex(tc->m);
		bool success = joinThread(tc->t);
		assert(success);
		deleteMutex(tc->m);
	}
	
	SoundManagerPrivate(double maxBufferTime):maxBufferTime(maxBufferTime){
		#ifdef LINUX_PLATFORM
		driver = new AlsaSoundDriver();
		#else
		#error Unsupported Operating System: Missing ISoundDriver implementation
		#endif
	}
	
	~SoundManagerPrivate(){
		for(auto it = c.begin(); it!=c.end(); ++it){
			stop(&(it->second));
		}
		delete driver;
	}
	
};

SoundManager::SoundManager(double maxBufferTime){
	p = new SoundManagerPrivate(maxBufferTime);
}
	
SoundManager::~SoundManager(){
	delete p;
}
	
void SoundManager::play(ISoundSource* source){
	auto it = p->c.find(source);
	if(it==p->c.end()){
		p->c[source] = SoundManagerPrivate::ThreadContext();
		SoundManagerPrivate::ThreadContext& tc = p->c[source];
		initMutex(tc.m);
		tc.driver = p->driver;
		tc.source = source;
		tc.mustExit = tc.isPause = false;
		tc.maxBufferTime = p->maxBufferTime;
		bool success = createThread(tc.t, SoundManagerPrivate::threadMain, &tc, true);
		assert(success);
	}else{
		lockMutex(it->second.m);
		it->second.isPause = false;
		unlockMutex(it->second.m);
	}
}

void SoundManager::pause(ISoundSource* source){
	auto it = p->c.find(source);
	if(it!=p->c.end()){
		lockMutex(it->second.m);
		it->second.isPause = true;
		unlockMutex(it->second.m);
	}
}

void SoundManager::stop(ISoundSource* source){
	auto it = p->c.find(source);
	if(it!=p->c.end()){
		p->stop(&(it->second));
		p->c.erase(it);
	}
}
