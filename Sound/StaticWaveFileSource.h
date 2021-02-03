#ifndef StaticWaveFileSource_H_INCLUDED
#define StaticWaveFileSource_H_INCLUDED

#include "ISoundSource.h"

#include <Threading.h>

//! A sound source for wave files loaded into memory
//! Currently only pcm wave files with 8 or 16 bit mono or stereo are supported
class StaticWaveFileSource : public ISoundSource{

	private:
	
	uint8_t* buffer;
	uint32_t bufferSize;
	
	ISoundSource::PCMFormat format;
	uint32_t sampleRate;
	uint32_t bytesPerFrame;
	
	bool good;
	
	Mutex m;
	bool loop;
	
	uint32_t offset;
	
	void parseBuffer();
	
	public:
	
	//! loads the wave file from a given path
	StaticWaveFileSource(const char* path);
	
	//! uses an externally provided buffer containing the whole wave file
	//! buffer will be deleted with delete[] in the destructor
	StaticWaveFileSource(uint8_t* buffer, uint32_t bufferSize);
	
	~StaticWaveFileSource();
	
	void setLoop(bool loop);
	
	//! true if wave file good and can be parsed
	bool isGood() const;
	
	ISoundSource::PCMFormat getPCMFormat() const;
	
	uint32_t getSampleRate() const;
	
	uint32_t fillNextBytes(uint8_t* buffer, uint32_t bufferSize);
	
};

#endif
