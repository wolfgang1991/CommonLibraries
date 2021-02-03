#ifndef SineWaveSoundSource_H_INCLUDED
#define SineWaveSoundSource_H_INCLUDED

#include "ISoundSource.h"

#include <Threading.h>

class SineWaveSoundSource : public ISoundSource{

	private:
	
	Mutex m;
	
	uint32_t frequency;
	
	const uint32_t sampleRate;
	
	double t;//current time;
	
	public:
	
	//! maxFrequency: frequency which is never exceeded by setFrequency
	SineWaveSoundSource(uint32_t frequency, uint32_t maxFrequency);
	
	~SineWaveSoundSource();
	
	void setFrequency(uint32_t frequency);
	
	uint32_t getFrequency();
	
	ISoundSource::PCMFormat getPCMFormat() const;
	
	uint32_t getSampleRate() const;
	
	uint32_t fillNextBytes(uint8_t* buffer, uint32_t bufferSize);
	
};

#endif
