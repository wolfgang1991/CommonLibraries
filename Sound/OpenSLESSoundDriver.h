#ifndef OpenSLESSoundDriver_H_INCLUDED
#define OpenSLESSoundDriver_H_INCLUDED

#include <platforms.h>

#ifdef ANDROID_PLATFORM// TODO other platforms suporting OpenSL ES

#include "ISoundDriver.h"

class OpenSLESSoundDriverPrivate;

class OpenSLESSoundDriver : public ISoundDriver{
	friend class OpenSLESSoundDriverPrivate;
	
	private:
	
	OpenSLESSoundDriverPrivate* p;
	
	public:
	
	OpenSLESSoundDriver();
	
	~OpenSLESSoundDriver();
	
	IPCMPlaybackContext* createPlaybackContext(ISoundSource* source);
	
	uint32_t getNextAcceptableSamplingFrequency(uint32_t desiredFrequency);
	
};

#endif

#endif
