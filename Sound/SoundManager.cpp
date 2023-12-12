#include "SoundManager.h"
#include "ISoundDriver.h"
#include "AlsaSoundDriver.h"
#include "OpenSLESSoundDriver.h"
#include "NoSoundDriver.h"

#include <Threading.h>
#include <platforms.h>
#include <timing.h>

#include <map>
#include <cassert>
#include <set>
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
		bool running = true;
		bool isPause = false;
		while(running){
			lockMutex(tc->m);
			isPause = tc->isPause;
			running = !tc->mustExit;
			unlockMutex(tc->m);
			uint32_t bytesUpdated = 0;
			double bufferTime = c->getDelayFrameCount()*secondsPerFrame;
			//std::cout << "bufferTime: " << bufferTime << std::endl;
			if(bufferTime<maxBufferTime){
				bytesUpdated = c->update(isPause, maxBytesUpdate);
			}
			if(bytesUpdated==0){delay(1);}//avoid busy wait if no update
		}
		delete c;
		return NULL;
	}
	
	std::map<ISoundSource*, ThreadContext> c;
	std::set<ISoundSource*> pool;
	
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
		#elif defined(ANDROID_PLATFORM)
		driver = new OpenSLESSoundDriver();
		#elif defined(__APPLE__) && defined(__MACH__)
		driver = new NoSoundDriver();
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
	for(auto it = p->pool.begin(); it!=p->pool.end(); ++it){//first stop everything then delete (because of SoundSource reuse e.g. in SoundEffectQueue)
		stop(*it);
	}
	for(auto it = p->pool.begin(); it!=p->pool.end(); ++it){
		delete *it;
	}
	delete p;
}
	
void SoundManager::play(ISoundSource* source){
	auto it = p->c.find(source);
	if(it==p->c.end()){
		source->seek(0.f);
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

ISoundDriver* SoundManager::getSoundDriver() const{
	return p->driver;
}

void SoundManager::update(){
	auto it=p->c.begin();
	while(it!=p->c.end()){
		if(it->first->isPlayingOrReady()){
			++it;
		}else{
			p->stop(&(it->second));
			p->c.erase(it);
			it = p->c.begin();
		}
	}
}

void SoundManager::addToPool(ISoundSource* src){
	p->pool.insert(src);
}

void SoundManager::deleteSource(ISoundSource* source){
	stop(source);
	delete source;
	p->pool.erase(source);
}
