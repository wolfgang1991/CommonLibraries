#ifndef AlsaSoundDriver_H_INCLUDED
#define AlsaSoundDriver_H_INCLUDED

#include <platforms.h>

#ifdef LINUX_PLATFORM

#include "ISoundDriver.h"

class AlsaSoundDriver : public ISoundDriver{
	
	public:
	
	AlsaSoundDriver(){}
	
	IPCMPlaybackContext* createPlaybackContext(ISoundSource* source);
	
	uint32_t getNextAcceptableSamplingFrequency(uint32_t desiredFrequency){
		return desiredFrequency;
	}
	
};

#endif

#endif
