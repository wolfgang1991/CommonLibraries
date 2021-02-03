#include "SineWaveSoundSource.h"

#include <Matrix.h>

#include <limits>
#include <iostream>

SineWaveSoundSource::SineWaveSoundSource(uint32_t frequency, uint32_t maxFrequency):sampleRate(2*maxFrequency),t(0){
	initMutex(m);
	setFrequency(frequency);
}

SineWaveSoundSource::~SineWaveSoundSource(){
	deleteMutex(m);
}

void SineWaveSoundSource::setFrequency(uint32_t frequency){
	uint32_t maxFrequency = sampleRate/2;
	lockMutex(m);
	this->frequency = frequency>maxFrequency?maxFrequency:frequency;
	unlockMutex(m);
}

uint32_t SineWaveSoundSource::getFrequency(){
	lockMutex(m);
	uint32_t f = frequency;
	unlockMutex(m);
	return f;
}

ISoundSource::PCMFormat SineWaveSoundSource::getPCMFormat() const{
	return ISoundSource::MONO8;
}

uint32_t SineWaveSoundSource::getSampleRate() const{
	return sampleRate;
}

uint32_t SineWaveSoundSource::fillNextBytes(uint8_t* buffer, uint32_t bufferSize){
	double currentTime = t;
	double timePerSample = 1.0/((double)sampleRate);
	lockMutex(m);
	double omega = 2.0*3.141592653589793238*frequency;
	std::cout << "frequency: " << frequency << " omega: " << omega << std::endl;
	unlockMutex(m);
	for(uint32_t i=0; i<bufferSize; i++){
		currentTime = t+((double)i)*timePerSample;
		double sample = sin(omega*currentTime);
		buffer[i] = rdScalar<double,uint8_t>(127.5+127.5*sample);
	}
	t = currentTime;
	return bufferSize;
}
