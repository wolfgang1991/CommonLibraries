#ifndef ISoundSource_H_INCLUDED
#define ISoundSource_H_INCLUDED

#include <cstdint>

//! Interface for a source of pcm data
//! the ISoundSource is usually used from a different thread than it is created / modified, implementation dependent synchonization may be necessary
class ISoundSource{
	
	public:
	
	enum PCMFormat{
		MONO8,//! uint8_t
		STEREO8,//! uint8_t
		MONO16,//! little endian int16_t
		STEREO16,//! little endian int16_t
		FORMAT_COUNT
	};
	
	virtual PCMFormat getPCMFormat() const = 0;
	
	virtual uint32_t getSampleRate() const = 0;
	
	virtual uint8_t getChannelCount() const{
		PCMFormat format = getPCMFormat();
		return format==MONO8||format==MONO16?1:2;
	}
	
	//! returns actually filled bytes in the given pcm format
	//! samples are "interleaved" in case of stereo
	//! this method MUST NOT return bad frames (e.g. just one sample in case of interleaved)
	virtual uint32_t fillNextBytes(uint8_t* buffer, uint32_t bufferSize) = 0;
	
	virtual ~ISoundSource(){}
	
};

#endif
