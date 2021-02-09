#ifndef StaticWaveFileSource_H_INCLUDED
#define StaticWaveFileSource_H_INCLUDED

#include "ISoundSource.h"

#include <Threading.h>

#include <functional>

//! A sound source for wave files loaded into memory
//! Currently only pcm wave files with 8 or 16 bit mono or stereo are supported
class StaticWaveFileSource : public ISoundSource{

	private:
	
	//read only:
	uint8_t* buffer;
	uint32_t bufferSize;
	
	ISoundSource::PCMFormat format;
	uint32_t sampleRate;
	uint32_t bytesPerFrame;
	
	bool good;
	
	//exchange:
	Mutex m;
	bool loop;
	bool ready;
	int32_t offsetToSet;//-1 if no change
	
	//thread only:
	uint32_t offset;
	
	bool deleteMemoryOnDestruct;
	
	void parseBuffer();
	
	public:
	
	//! loads the wave file from a given path
	StaticWaveFileSource(SoundManager* soundmgr, const char* path);
	
	//! loads the wave file using a loader function, the loader function must return true on success and read the file into a new buffer (first param) and fills the buffer size (second param)
	StaticWaveFileSource(SoundManager* soundmgr, const std::function<bool(char*&, uint32_t&)>& loadFunction);
	
	//! uses an externally provided buffer containing the whole wave file
	StaticWaveFileSource(SoundManager* soundmgr, uint8_t* buffer, uint32_t bufferSize, bool useForeignMemory = true, bool deleteMemoryOnDestruct = true);
	
	~StaticWaveFileSource();
	
	void setLoop(bool loop);
	
	//! true if wave file good and can be parsed
	bool isGood() const;
	
	ISoundSource::PCMFormat getPCMFormat() const;
	
	uint32_t getSampleRate() const;
	
	uint32_t fillNextBytes(uint8_t* buffer, uint32_t bufferSize);
	
	bool isPlayingOrReady();
	
	void seek(float position);
	
};

#endif
