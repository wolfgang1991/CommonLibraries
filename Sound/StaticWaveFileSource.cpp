#include "StaticWaveFileSource.h"
#include "ISoundDriver.h"
#include "SoundManager.h"

#include <BitFunctions.h>

#include <fstream>
#include <iostream>

#define WAVE_HEADER_SIZE 44

void StaticWaveFileSource::parseBuffer(){
	good = bufferSize>WAVE_HEADER_SIZE;
	if(good){
		good = strncmp((const char*)buffer, "RIFF", 4)==0;
		if(good){
			good = strncmp((const char*)&(buffer[8]), "WAVEfmt ", 8)==0;
			uint32_t offset = 22;
			uint16_t channelCount = readLittleEndian<uint16_t>(buffer, offset);
			sampleRate = soundmgr->getSoundDriver()->getNextAcceptableSamplingFrequency(readLittleEndian<uint32_t>(buffer, offset));
			offset = 34;
			uint16_t bitsPerSample = readLittleEndian<uint16_t>(buffer, offset);
			good = (bitsPerSample==8 || bitsPerSample==16) && (channelCount==1 || channelCount==2) && sampleRate<50000;//more than 50000 not plausible
			if(good){
				this->offset = WAVE_HEADER_SIZE;
				if(channelCount==1){
					format = bitsPerSample==8?MONO8:MONO16;
				}else if(channelCount==2){
					format = bitsPerSample==8?STEREO8:STEREO16;
				}
				bytesPerFrame = channelCount*bitsPerSample/8;
			}else{
				std::cerr << "Unsupported pcm format: bitsPerSample=" << bitsPerSample << " channelCount=" << channelCount << " sampleRate=" << sampleRate << std::endl;
			}
		}
	}
	if(!good){
		std::cerr << "Error while parsing wave file." << std::endl;
		delete[] buffer;
		buffer = NULL;
	}
}

StaticWaveFileSource::StaticWaveFileSource(SoundManager* soundmgr, const char* path):ISoundSource(soundmgr){
	std::cout << "Loading wave file: " << path << std::endl;
	initMutex(m);
	buffer = NULL;
	loop = false;
	std::ifstream fp(path, std::ifstream::binary);
	good = fp.good();
	if(good){
		const auto begin = fp.tellg();
		fp.seekg(0, std::ios::end);
		const auto end = fp.tellg();
		bufferSize = (end-begin);
		fp.seekg(0, std::ios::beg);
		good = bufferSize>WAVE_HEADER_SIZE;
		if(good){
			buffer = new uint8_t[bufferSize];
			fp.read((char*)buffer, bufferSize);
			parseBuffer();
		}
	}
	ready = good;
	offsetToSet = -1;
	deleteMemoryOnDestruct = true;
}

StaticWaveFileSource::StaticWaveFileSource(SoundManager* soundmgr, uint8_t* buffer, uint32_t bufferSize, bool useForeignMemory, bool deleteMemoryOnDestruct):ISoundSource(soundmgr){
	this->deleteMemoryOnDestruct = deleteMemoryOnDestruct;
	good = true;
	initMutex(m);
	loop = false;
	this->bufferSize = bufferSize;
	if(useForeignMemory){
		this->buffer = buffer;
	}else{
		this->buffer = new uint8_t[bufferSize];
		memcpy(this->buffer, buffer, bufferSize);
	}
	parseBuffer();
}

StaticWaveFileSource::StaticWaveFileSource(SoundManager* soundmgr, const std::function<bool(char*&, uint32_t&)>& loadFunction):ISoundSource(soundmgr){
	deleteMemoryOnDestruct = true;
	char* buf;
	good = loadFunction(buf, bufferSize);
	buffer = (uint8_t*)buf;
	initMutex(m);
	loop = false;
	parseBuffer();
}

StaticWaveFileSource::~StaticWaveFileSource(){
	if(good && deleteMemoryOnDestruct){
		delete[] buffer;
	}
	deleteMutex(m);
}

void StaticWaveFileSource::setLoop(bool loop){
	lockMutex(m);
	this->loop = loop;
	ready = good && (ready || loop);
	unlockMutex(m);
}

bool StaticWaveFileSource::isGood() const{
	return good;
}

ISoundSource::PCMFormat StaticWaveFileSource::getPCMFormat() const{
	return format;
}

uint32_t StaticWaveFileSource::getSampleRate() const{
	return sampleRate;
}

uint32_t StaticWaveFileSource::fillNextBytes(uint8_t* buffer, uint32_t bufferSize){
	if(good){
		lockMutex(m);
		bool isLoop = loop;
		if(offsetToSet>=0){
			offset = offsetToSet;
			offsetToSet = -1;
		}
		unlockMutex(m);
		if(offset==this->bufferSize){
			if(isLoop){
				offset = WAVE_HEADER_SIZE;
			}else{
				lockMutex(m);
				ready = false;
				unlockMutex(m);
			}
		}
		uint32_t frameCount = bufferSize/bytesPerFrame;//integer division
		uint32_t bytesRemaining = this->bufferSize-offset;
		uint32_t bytesToCopy = frameCount*bytesPerFrame;
		if(bytesToCopy>bytesRemaining){bytesToCopy = bytesRemaining;}
		if(bytesToCopy>0){
			memcpy(buffer, &(this->buffer[offset]), bytesToCopy);
			offset += bytesToCopy;
		}
		//std::cout << "bytesToCopy: " << bytesToCopy << std::endl;
		return bytesToCopy;
	}
	return 0;
}

bool StaticWaveFileSource::isPlayingOrReady(){
	bool result = false;
	lockMutex(m);
	result = ready;
	unlockMutex(m);
	return result;
}

void StaticWaveFileSource::seek(float position){
	uint32_t ipos = position*bufferSize;
	uint32_t frameCount = ipos/bytesPerFrame;//integer divisiopn
	lockMutex(m);
	offsetToSet = frameCount*bytesPerFrame;
	ready = true;
	unlockMutex(m);
}
