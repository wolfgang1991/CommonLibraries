#ifndef SoundManager_H_INCLUDED
#define SoundManager_H_INCLUDED

#include "ISoundSource.h"

class SoundManagerPrivate;
class ISoundDriver;

class SoundManager{
	friend class SoundManagerPrivate;

	private:
	
	SoundManagerPrivate* p;
	
	void addToPool(ISoundSource* src);
	
	public:
	
	SoundManager(double maxBufferTime = 0.1);
	
	~SoundManager();
	
	ISoundDriver* getSoundDriver() const;
	
	//! plays a sound (initializes backend if necessary)
	void play(ISoundSource* source);
	
	void pause(ISoundSource* source);
	
	//! completely deallocate everything related
	void stop(ISoundSource* source);
	
	//! should be regularly called to clean up old data (e.g. stops finished sounds)
	void update();
	
	//! creates a new sound source with first argument == this and adds it to the pool for memory management
	template<typename T, typename... Args>
	T* create(Args... args){
		T* src = new T(this, args...);
		addToPool(src);
		return src;
	}
	
	//! stops and deletes a source explicitly from the pool
	void deleteSource(ISoundSource* source);
	
};

#endif
