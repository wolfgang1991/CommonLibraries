#include "SineWaveSoundSource.h"
#include "ISoundDriver.h"
#include "SoundManager.h"

#include <Matrix.h>

#include <limits>
#include <iostream>

SineWaveSoundSource::SineWaveSoundSource(SoundManager* soundmgr, uint32_t frequency, uint32_t maxFrequency):ISoundSource(soundmgr),sampleRate(soundmgr->getSoundDriver()->getNextAcceptableSamplingFrequency(2*maxFrequency)),lastPhase(0){
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

static const double twoPi = 2.0*3.141592653589793238;

uint32_t SineWaveSoundSource::fillNextBytes(uint8_t* buffer, uint32_t bufferSize){
	double currentTime = 0.0;
	double timePerSample = 1.0/((double)sampleRate);
	lockMutex(m);
	double omega = twoPi*frequency;
	//std::cout << "frequency: " << frequency << " omega: " << omega << " lastPhase: " << lastPhase << std::endl;
	unlockMutex(m);
	for(uint32_t i=0; i<bufferSize; i++){
		currentTime = ((double)i)*timePerSample;
		double sample = sin(lastPhase+omega*currentTime);
		buffer[i] = rdScalar<double,uint8_t>(127.5+127.5*sample);
		//std::cerr << sample << std::endl;
	}
	currentTime = ((double)bufferSize)*timePerSample;
	lastPhase = fmod(lastPhase+omega*currentTime,twoPi);
	//lastPhase += omega*currentTime;
	return bufferSize;
}

bool SineWaveSoundSource::isPlayingOrReady(){
	return true;
}
