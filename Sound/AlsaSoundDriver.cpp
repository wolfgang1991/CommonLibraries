#include "AlsaSoundDriver.h"

#ifdef LINUX_PLATFORM

#include "ISoundSource.h"

#include <alsa/asoundlib.h>
#include <iostream>

#define PCM_DEVICE "default"

static snd_pcm_format_t formatIds[ISoundSource::FORMAT_COUNT] = {
	SND_PCM_FORMAT_U8,//MONO8
	SND_PCM_FORMAT_U8,//STEREO8
	SND_PCM_FORMAT_S16_LE,//MONO16
	SND_PCM_FORMAT_S16_LE//STEREO16
};

static unsigned int channelNumbers[ISoundSource::FORMAT_COUNT] = {
	1,//MONO8
	2,//STEREO8
	1,//MONO16
	2//STEREO16
};

static unsigned int singleSampleSizes[ISoundSource::FORMAT_COUNT] = {
	1,//MONO8
	1,//STEREO8
	2,//MONO16
	2//STEREO16
};

class AlsaPCMPlaybackContext : public IPCMPlaybackContext{

	public:
	
	snd_pcm_t* pcm_handle;
	snd_pcm_hw_params_t* params;
	snd_pcm_uframes_t frames;
	
	uint8_t* buffer;
	uint32_t bufferSize;
	
	uint32_t bytesPerFrame;
	
	AlsaPCMPlaybackContext(ISoundSource* source):IPCMPlaybackContext(source){
		int result;
		//Open the PCM device in playback mode
		if((result = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0){
			std::cerr << "ERROR: Unable to open PCM device: " << PCM_DEVICE << " (error: " << snd_strerror(result) << ")" << std::endl;
		}
		//Set parameters
		snd_pcm_hw_params_malloc(&params);
		snd_pcm_hw_params_any(pcm_handle, params);
		ISoundSource::PCMFormat format = source->getPCMFormat();
		if((result = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0){
			std::cerr << "ERROR: Can't set interleaved mode (error: " << snd_strerror(result) << ")." << std::endl;
		}
		if((result = snd_pcm_hw_params_set_format(pcm_handle, params, formatIds[format])) < 0){
			std::cerr << "ERROR: Can't set format (error: " << snd_strerror(result) << ")." << std::endl;
		}
		if((result = snd_pcm_hw_params_set_channels(pcm_handle, params, channelNumbers[format])) < 0){
			std::cerr << "ERROR: Can't set channels number (error: " << snd_strerror(result) << ")." << std::endl;
		}
		unsigned int rate = source->getSampleRate();
		if((result = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0){
			std::cerr << "ERROR: Can't set rate (error: " << snd_strerror(result) << ")." << std::endl;
		}
		if((result = snd_pcm_hw_params(pcm_handle, params)) < 0){
			std::cerr << "ERROR: Can't set harware parameters (error: " << snd_strerror(result) << ")." << std::endl;
		}
		//Allocate buffer
		snd_pcm_hw_params_get_period_size(params, &frames, 0);
		bytesPerFrame = channelNumbers[format] * singleSampleSizes[format];
		bufferSize = frames * bytesPerFrame;
		buffer = new uint8_t[bufferSize];
	}
	
	~AlsaPCMPlaybackContext(){
		delete[] buffer;
		snd_pcm_hw_params_free(params);
		snd_pcm_close(pcm_handle);
		snd_config_update_free_global();
	}
	
	uint32_t update(bool pause, uint32_t maxBytes){
		if(maxBytes==0){return 0;}
		if(maxBytes>bufferSize){maxBytes = bufferSize;}
		uint32_t filledBytes;
		if(pause){
			filledBytes = maxBytes;
			memset(buffer, 0, maxBytes);//TODO notice pause change and fill last frame once when switched to pause
		}else{
			filledBytes = source->fillNextBytes(buffer, maxBytes);
		}
		if(filledBytes>0){
			snd_pcm_uframes_t actualFrames = filledBytes/bytesPerFrame;
			int result = snd_pcm_writei(pcm_handle, buffer, actualFrames);
			if(result == -EPIPE){
				std::cerr << "Underrun occured" << std::endl;
				snd_pcm_prepare(pcm_handle);
			}else if(result<0){
				std::cerr << "ERROR: Can't write to PCM device (error: " << snd_strerror(result) << ")." << std::endl;
			}
		}
//		std::cout << "snd_pcm_avail: " << snd_pcm_avail(pcm_handle) << std::endl;
		return filledBytes;
	}
	
	uint32_t getDelayFrameCount(){
		snd_pcm_sframes_t delayFrames;
		int result;
		if((result = snd_pcm_delay(pcm_handle, &delayFrames)) < 0){
			delayFrames = 0;
			std::cerr << "Can't get pcm delay (" << snd_strerror(result) << ") (probably nothing playing yet?)." << std::endl;
		}
		return delayFrames;
	}
	
};

IPCMPlaybackContext* AlsaSoundDriver::createPlaybackContext(ISoundSource* source){
	return new AlsaPCMPlaybackContext(source);
}


#endif
