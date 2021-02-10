#include "OpenSLESSoundDriver.h"

#ifdef ANDROID_PLATFORM // TODO other platforms suporting OpenSL ES

#include "ISoundSource.h"

#include <Threading.h>

#include <SLES/OpenSLES.h>

#ifdef ANDROID_PLATFORM
#include <android/log.h>
#include <SLES/OpenSLES_Android.h>
#else
#include <cstdio>
#endif

#include <csignal>

#define QUEUED_BUFFERS 2

static bool slassert(SLresult result, const char* message = "-"){
	bool success = result==SL_RESULT_SUCCESS;
	if(!success){
		#ifdef ANDROID_PLATFORM
		__android_log_print(ANDROID_LOG_ERROR, "assertion failure", "result: %i, at: %s", result, message);
		#else
		printf("assertion failure: result: %i, at: %s\n", result, message);
		#endif
		raise(SIGINT);
	}
	return success;
}

class OpenSLESSoundDriverPrivate{
	
	public:
	
	SLObjectItf engine_obj;
	SLEngineItf engine;
	
	SLObjectItf output_mix_obj;
	SLVolumeItf output_mix_vol;
	
	bool good;//true if init (contructor) successful
	
	Mutex apiMutex;//TODO just one API at once call via mutex, required for thread safety
	
	OpenSLESSoundDriverPrivate(){
		SLresult result;
 		result = slCreateEngine(&engine_obj,0,NULL,0,NULL,NULL);
 		if(slassert(result, "slCreateEngine")){
			result = (*engine_obj)->Realize(engine_obj, SL_BOOLEAN_FALSE);
			if(slassert(result, "engine_obj.Realize")){
				result = (*engine_obj)->GetInterface(engine_obj, SL_IID_ENGINE, &engine);
				if(slassert(result, "engine_obj.GetInterface")){
					//create the main OutputMix, try to get a volume interface for it
					const SLInterfaceID ids[] = { SL_IID_VOLUME };
					const SLboolean req[] = { SL_BOOLEAN_FALSE };
					result = (*engine)->CreateOutputMix( engine, &output_mix_obj, 1, ids, req );
					if(slassert(result, "engine.CreateOutputMix")){
						result = (*output_mix_obj)->Realize( output_mix_obj, SL_BOOLEAN_FALSE );
						if(slassert(result, "output_mix_obj.Realize")){
							if((*output_mix_obj)->GetInterface(output_mix_obj, SL_IID_VOLUME, &output_mix_vol ) != SL_RESULT_SUCCESS){
								 output_mix_vol = NULL;
							}
						}
					}
				}
			}
		}
		initMutex(apiMutex);
		good = result==SL_RESULT_SUCCESS;
		#ifdef ANDROID_PLATFORM
		__android_log_print(ANDROID_LOG_VERBOSE, "OpenSL init", "good: %i", (int)good);
		#else
		printf("OpenSL init: good: %i\n", (int)good);
		#endif
	}
	
	~OpenSLESSoundDriverPrivate(){
		if(good){
			(*output_mix_obj)->Destroy(output_mix_obj);
			(*engine_obj)->Destroy(engine_obj);
		}
		deleteMutex(apiMutex);
	}
	
};

static const SLuint32 sourceFormat2BitsPerSample[ISoundSource::FORMAT_COUNT] = {
	SL_PCMSAMPLEFORMAT_FIXED_8,
	SL_PCMSAMPLEFORMAT_FIXED_8,
	SL_PCMSAMPLEFORMAT_FIXED_16,
	SL_PCMSAMPLEFORMAT_FIXED_16
};

static const SLuint32 sourceFormat2ContainerSize[ISoundSource::FORMAT_COUNT] = {8,8,16,16};

static const SLuint32 sourceFormat2ChannelMask[ISoundSource::FORMAT_COUNT] = {
	SL_PCMSAMPLEFORMAT_FIXED_8,
	SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,
	SL_PCMSAMPLEFORMAT_FIXED_8,
	SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT
};

static const uint32_t bytesPerFrame[ISoundSource::FORMAT_COUNT] = {1, 2, 2, 4};

class OpenSLESPlaybackContext : public IPCMPlaybackContext{

	private:
	
	#ifdef ANDROID_PLATFORM
	SLDataLocator_AndroidSimpleBufferQueue in_loc;
	#else
	SLDataLocator_Address in_loc;
	#endif
	
	SLDataFormat_PCM format;
	
	SLDataSource src;
	
	SLDataLocator_OutputMix out_loc;
	
	SLDataSink dst;
	
	SLObjectItf player_obj;
	SLPlayItf player;
	SLVolumeItf player_vol;
	 
	#ifdef ANDROID_PLATFORM
	SLAndroidSimpleBufferQueueItf player_buf_q;
	#endif
	
	Mutex cbkMutex;
	int32_t queuedBuffers;

	OpenSLESSoundDriverPrivate* p;
	
	bool good;
	
	struct QueuedBuffer{
		uint8_t* buffer;
		uint32_t bufferSize;
		uint32_t filledSize;
	};
	
	QueuedBuffer buffers[QUEUED_BUFFERS];
	uint32_t nextToFillIndex;
	
	public:
	
	static void SLAPIENTRY play_callback(SLPlayItf player, void *context, SLuint32 event){
		__android_log_print(ANDROID_LOG_ERROR, "OpenSL driver", "Buffer underrun???");
	}
	
	static void SLAPIENTRY unqueueCallback(SLAndroidSimpleBufferQueueItf caller, void* pContext){
		OpenSLESPlaybackContext* c = (OpenSLESPlaybackContext*)pContext;
		lockMutex(c->cbkMutex);
		c->queuedBuffers--;
		unlockMutex(c->cbkMutex);
	}
	
	OpenSLESPlaybackContext(ISoundSource* source, OpenSLESSoundDriverPrivate* p):IPCMPlaybackContext(source),p(p){
		ISoundSource::PCMFormat sf = source->getPCMFormat();
		nextToFillIndex = 0;
		uint32_t bufferSize = source->getSampleRate()*bytesPerFrame[sf];//for 1 second
		for(uint32_t i=0; i<QUEUED_BUFFERS; i++){
			buffers[i] = QueuedBuffer{new uint8_t[bufferSize], bufferSize, 0};
		}
		#ifdef ANDROID_PLATFORM
		in_loc.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
		in_loc.numBuffers = QUEUED_BUFFERS;
		#else
		#error non Android OpenSL ES code missing
		#endif
		format.formatType = SL_DATAFORMAT_PCM;
		format.numChannels = source->getChannelCount();
		format.samplesPerSec = source->getSampleRate() * 1000; //milliHz
		format.bitsPerSample = sourceFormat2BitsPerSample[sf];
		format.containerSize = sourceFormat2ContainerSize[sf];
		format.channelMask = sourceFormat2ChannelMask[sf];
		format.endianness = SL_BYTEORDER_LITTLEENDIAN;
		src.pLocator = &in_loc;
		src.pFormat = &format;
		out_loc.locatorType = SL_DATALOCATOR_OUTPUTMIX;
		out_loc.outputMix = p->output_mix_obj;
		dst.pLocator = &out_loc;
		dst.pFormat = NULL;
		queuedBuffers = 0;
		initMutex(cbkMutex);
		#ifdef ANDROID_PLATFORM
		const SLInterfaceID ids[] = { SL_IID_VOLUME, SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
		const SLboolean req[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
		#else
		const SLInterfaceID ids[] = { SL_IID_VOLUME };
		const SLboolean req[] = { SL_BOOLEAN_TRUE };
		#endif
		if(p->good){
			SLresult result;
			lockMutex(p->apiMutex);
			result = (*p->engine)->CreateAudioPlayer(p->engine, &player_obj, &src, &dst, sizeof(ids)/sizeof(ids[0]), ids, req);
			if(slassert(result, "engine.CreateAudioPlayer")){
				result = (*player_obj)->Realize(player_obj, SL_BOOLEAN_FALSE);
				if(slassert(result, "player_obj.Realize")){
					result = (*player_obj)->GetInterface(player_obj, SL_IID_PLAY, &player);
					if(slassert(result, "player_obj.GetInterface SL_IID_PLAY")){
						result = (*player_obj)->GetInterface(player_obj, SL_IID_VOLUME, &player_vol);
						if(slassert(result, "player_obj.GetInterface SL_IID_VOLUME")){
							#ifdef ANDROID_PLATFORM
							result = (*player_obj)->GetInterface(player_obj,SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &player_buf_q);
							if(slassert(result, "player_obj.GetInterface SL_IID_ANDROIDSIMPLEBUFFERQUEUE")){
							#endif
							result = (*player_buf_q)->RegisterCallback(player_buf_q, unqueueCallback, this);
							if(slassert(result, "player_buf_q.RegisterCallback")){
								result = (*player)->RegisterCallback(player, play_callback, this);
								if(slassert(result, "player.RegisterCallback")){
									result = (*player)->SetCallbackEventsMask(player, SL_PLAYEVENT_HEADATEND);
									slassert(result, "player.SetCallbackEventsMask");
								}
							}
							#ifdef ANDROID_PLATFORM
							}
							#endif
						}
					}
				}
			}
			unlockMutex(p->apiMutex);
		}
	}
	
	~OpenSLESPlaybackContext(){
		lockMutex(p->apiMutex);
		(*player_obj)->Destroy(player_obj);
		unlockMutex(p->apiMutex);
		deleteMutex(cbkMutex);
		for(uint32_t i=0; i<QUEUED_BUFFERS; i++){
			delete[] buffers[i].buffer;
		}
	}
	
	uint32_t update(bool pause, uint32_t maxBytes){
		if(maxBytes>2){
			lockMutex(cbkMutex);
			bool mustEnqueue = queuedBuffers<QUEUED_BUFFERS;
			if(mustEnqueue){maxBytes = maxBytes/(QUEUED_BUFFERS-queuedBuffers);}//maxBytes per buffer to fill
			bool mustSetPlayState = queuedBuffers<=0;
			unlockMutex(cbkMutex);
			while(mustEnqueue){
				QueuedBuffer& b = buffers[nextToFillIndex];
				nextToFillIndex = (nextToFillIndex+1)%QUEUED_BUFFERS;
				b.filledSize = pause?0:(source->fillNextBytes(b.buffer, maxBytes<b.bufferSize?maxBytes:b.bufferSize));
				if(b.filledSize==0){//fill silence
					b.filledSize = source->alignBytesToFrames(maxBytes<b.bufferSize?maxBytes:b.bufferSize);
					memset(b.buffer, 0, b.filledSize);
				}
				#ifdef ANDROID_PLATFORM
				lockMutex(p->apiMutex);
				SLresult result = (*player_buf_q)->Enqueue(player_buf_q, b.buffer, b.filledSize);
				unlockMutex(p->apiMutex);
				slassert(result, "player_buf_q.Enqueue");
				#endif
				lockMutex(cbkMutex);
				queuedBuffers++;
				mustEnqueue = queuedBuffers<2;
				unlockMutex(cbkMutex);
				uint32_t frameCount = getDelayFrameCount();
				__android_log_print(ANDROID_LOG_ERROR, "b.filledSize", "b.filledSize: %i frameCount: %i, delay: %f", (int)b.filledSize, (int)frameCount, ((double)frameCount)/source->getSampleRate());
			}
			if(mustSetPlayState){
				lockMutex(p->apiMutex);
				SLresult result = (*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING);
				unlockMutex(p->apiMutex);
				slassert(result, "player.SetPlayState SL_PLAYSTATE_PLAYING");
			}
		}
		return 0;
	}
	
	uint32_t getDelayFrameCount(){
		uint32_t byteCount = 0;
		lockMutex(cbkMutex);
		for(int32_t i=0; i<(int32_t)queuedBuffers; i++){
			int32_t index = ((int32_t)nextToFillIndex)-i-1;
			if(index<0){index = QUEUED_BUFFERS+index;}
			byteCount += buffers[index].filledSize;
			//__android_log_print(ANDROID_LOG_ERROR, "test", "i: %i index: %i, filledSize: %i", (int)i, (int)index, (int)(buffers[index].filledSize));
		}
		unlockMutex(cbkMutex);
		//__android_log_print(ANDROID_LOG_ERROR, "byteCount", "byteCount: %i", (int)byteCount);
		return byteCount/bytesPerFrame[source->getPCMFormat()];
	}
	
};

OpenSLESSoundDriver::OpenSLESSoundDriver(){
	p = new OpenSLESSoundDriverPrivate();
}
	
OpenSLESSoundDriver::~OpenSLESSoundDriver(){
	delete p;
}
	
IPCMPlaybackContext* OpenSLESSoundDriver::createPlaybackContext(ISoundSource* source){
	return new OpenSLESPlaybackContext(source, p);
}

static const uint32_t accSampFreq[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
static const uint32_t accSampFreqCount = sizeof(accSampFreq)/sizeof(accSampFreq[0]);

uint32_t OpenSLESSoundDriver::getNextAcceptableSamplingFrequency(uint32_t desiredFrequency){
	for(uint32_t i=0; i<accSampFreqCount; i++){
		if(accSampFreq[i]>=desiredFrequency){
			return accSampFreq[i];
		}
	}
	return accSampFreq[accSampFreqCount-1];
}

#endif
