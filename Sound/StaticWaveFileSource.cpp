#include "StaticWaveFileSource.h"
#include "ISoundDriver.h"
#include "SoundManager.h"

#include <BitFunctions.h>

#include <fstream>
#include <iostream>

#define MIN_WAVE_HEADER_SIZE 44
#define MIN_FMT_CHUNK_SIZE 24

//! not found if offset==bufferSize
static uint32_t findChunk(const char* buffer, uint32_t bufferSize, uint32_t offset, const std::string& chunkBeginning){
	bool found = false;
	for(;offset<bufferSize;offset++){
		 found = strncmp(&(buffer[offset]), chunkBeginning.c_str(), chunkBeginning.size())==0;
		 if(found){return offset;}
	}
	return offset;
}

void StaticWaveFileSource::parseBuffer(){
	good = bufferSize>MIN_WAVE_HEADER_SIZE;
	if(good){
		good = strncmp((const char*)buffer, "RIFF", 4)==0 && strncmp((const char*)&(buffer[8]), "WAVE", 4)==0;
		if(good){
			uint32_t offset = findChunk((const char*)buffer, bufferSize, 12, "fmt ");
			if(offset<bufferSize-MIN_FMT_CHUNK_SIZE){
				offset += 10;//channel offset inside fmt chunk
				uint16_t channelCount = readLittleEndian<uint16_t>(buffer, offset);
				sampleRate = soundmgr->getSoundDriver()->getNextAcceptableSamplingFrequency(readLittleEndian<uint32_t>(buffer, offset));
				offset += 6;
				uint16_t bitsPerSample = readLittleEndian<uint16_t>(buffer, offset);
				good = (bitsPerSample==8 || bitsPerSample==16) && (channelCount==1 || channelCount==2) && sampleRate<100000;//more than 100000 not plausible
				std::cout << "bitsPerSample=" << bitsPerSample << " channelCount=" << channelCount << " sampleRate=" << sampleRate << std::endl;//useful for compatibility checks
				if(good){
					this->offset = dataOffset = findChunk((const char*)buffer, bufferSize, offset, "data") + 8;
					if(channelCount==1){
						format = bitsPerSample==8?MONO8:MONO16;
					}else if(channelCount==2){
						format = bitsPerSample==8?STEREO8:STEREO16;
					}
					bytesPerFrame = channelCount*bitsPerSample/8;
				}else{
					std::cerr << "Unsupported pcm format." << std::endl;
				}
			}else{
				std::cerr << "\"fmt \" chunk not found or invalid." << std::endl;
			}
		}else{
			std::cerr << "RIFF / WAVE header identification missing." << std::endl;
		}
	}else{
		std::cerr << "Wave too small." << std::endl;
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
	offset = dataOffset = 0;
	format = ISoundSource::MONO8;
	sampleRate = 8000;
	bytesPerFrame = 1;
	std::ifstream fp(path, std::ifstream::binary);
	good = fp.good();
	if(good){
		const auto begin = fp.tellg();
		fp.seekg(0, std::ios::end);
		const auto end = fp.tellg();
		bufferSize = (end-begin);
		fp.seekg(0, std::ios::beg);
		buffer = new uint8_t[bufferSize];
		fp.read((char*)buffer, bufferSize);
		parseBuffer();
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
		if(offset>=this->bufferSize){
			if(isLoop){
				offset = dataOffset;
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
	offsetToSet = dataOffset + frameCount*bytesPerFrame;
	ready = true;
	unlockMutex(m);
}
