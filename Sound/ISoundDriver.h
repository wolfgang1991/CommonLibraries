#ifndef ISoundDriver_H_INCLUDED
#define ISoundDriver_H_INCLUDED

#include <cstdint>

class ISoundSource;
class IPCMPlaybackContext;

//! A sound driver has actual platform dependent code for actually playing sounds
class ISoundDriver{
	
	public:
	
	virtual ~ISoundDriver(){}
	
	//! created the driver dependent context (may be called from different threads)
	virtual IPCMPlaybackContext* createPlaybackContext(ISoundSource* source) = 0;
	
	//! return the next higher sampling frequency which is supported by the driver / underlying hardware
	virtual uint32_t getNextAcceptableSamplingFrequency(uint32_t desiredFrequency) = 0;
	
};

// driver dependent required structures for ISoundSource playpack, this needs to be extended for each sound driver
class IPCMPlaybackContext{
	
	protected:
	
	ISoundSource* source;
	
	public:
	
	IPCMPlaybackContext(ISoundSource* source):source(source){}
	
	virtual ~IPCMPlaybackContext(){}
	
	//! feeds pcm data into the driver dependent structures, blocks until all data is played or fed into internal buffers
	//! if pause no new data shall be pulled from the ISoundSource
	//! returns the amount of updated bytes
	//! maxBytes: the maximum amount of bytes which shall be written (useful to minimize the delay)
	virtual uint32_t update(bool pause, uint32_t maxBytes) = 0;
	
	//! gets the amount of buffered bytes which cause a delay in playback
	virtual uint32_t getDelayFrameCount() = 0;
	
};

#endif
