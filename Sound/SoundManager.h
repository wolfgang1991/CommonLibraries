#ifndef SoundManager_H_INCLUDED
#define SoundManager_H_INCLUDED

#include "ISoundSource.h"

class SoundManagerPrivate;

class SoundManager{
	friend class SoundManagerPrivate;

	private:
	
	SoundManagerPrivate* p;
	
	public:
	
	SoundManager(double maxBufferTime = 0.1);
	
	~SoundManager();
	
	//! plays a sound (initializes backend if necessary)
	void play(ISoundSource* source);
	
	void pause(ISoundSource* source);
	
	//! completely deallocate everything related
	void stop(ISoundSource* source);
	
};

#endif
