#ifndef AlsaSoundDriver_H_INCLUDED
#define AlsaSoundDriver_H_INCLUDED

#include <platforms.h>

#ifdef LINUX_PLATFORM

#include "ISoundDriver.h"

class AlsaSoundDriver : public ISoundDriver{
	
	public:
	
	AlsaSoundDriver(){}
	
	IPCMPlaybackContext* createPlaybackContext(ISoundSource* source);
	
};

#endif

#endif
