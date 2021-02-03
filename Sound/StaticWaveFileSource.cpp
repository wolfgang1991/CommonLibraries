#include "StaticWaveFileSource.h"

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
			sampleRate = readLittleEndian<uint32_t>(buffer, offset);
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

StaticWaveFileSource::StaticWaveFileSource(const char* path){
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
}

StaticWaveFileSource::StaticWaveFileSource(uint8_t* buffer, uint32_t bufferSize){
	initMutex(m);
	loop = false;
	this->buffer = buffer;
	this->bufferSize = bufferSize;
	parseBuffer();
}

StaticWaveFileSource::~StaticWaveFileSource(){
	if(good){
		delete[] buffer;
	}
	deleteMutex(m);
}

void StaticWaveFileSource::setLoop(bool loop){
	lockMutex(m);
	this->loop = loop;
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
		uint32_t frameCount = bufferSize/bytesPerFrame;//integer division
		uint32_t bytesRemaining = this->bufferSize-offset;
		uint32_t bytesToCopy = frameCount*bytesPerFrame;
		if(bytesToCopy>bytesRemaining){bytesToCopy = bytesRemaining;}
		if(bytesToCopy>0){
			memcpy(buffer, &(this->buffer[offset]), bytesToCopy);
			offset += bytesToCopy;
		}
		if(offset==this->bufferSize){
			lockMutex(m);
			if(loop){
				offset = WAVE_HEADER_SIZE;
			}
			unlockMutex(m);
		}
		//std::cout << "bytesToCopy: " << bytesToCopy << std::endl;
		return bytesToCopy;
	}
	return 0;
}
