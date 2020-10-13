#ifndef BITFUNCTIONS_H_INCLUDED
#define BITFUNCTIONS_H_INCLUDED

#include "platforms.h"

#ifndef MICROCONTROLLER_PLATFORM
#include <vector>
#endif
#include <string.h>
#include <stdint.h>

template <typename T>
bool getBit(const T value, const unsigned char index){
	return (bool)((value >> index) & (T)1);
}

template <typename T>
T setBit(const T value, const unsigned char index, const bool bit){
	if(bit){
		return value | (((T)1) << index);
	}else{
		return value & ~(((T)1) << index);
	}
}

inline bool getBitFromMultiBytes(uint32_t bitIndex, uint8_t* multiBytes){
	return getBit(multiBytes[bitIndex/8], bitIndex%8);
}

inline void setBitInMultiBytes(uint32_t bitIndex, uint8_t* multiBytes, const bool bit){
	uint32_t byteIndex = bitIndex/8;
	multiBytes[byteIndex] = setBit(multiBytes[byteIndex], bitIndex%8, bit);
}

template <typename T>
T getBitVector(const T value, const unsigned char startIndex, const unsigned char len){
	const T mask = (((T)1) << len)-((T)1);
	return (value >> startIndex) & mask;
}

template <typename T>
T setBitVector(const T value, const unsigned char startIndex, const unsigned char len, const T bitVector){
	const T mask = (((T)1) << len)-((T)1);
	return (value & ~(mask << startIndex)) | ((bitVector & mask) << startIndex);
}

//! Unaligned value read
//! changes offset by the amount of read bytes
template<typename T>
T readUnaligned(const char* buffer, uint32_t& offset){
	T value;
	memcpy(&value, &(buffer[offset]), sizeof(T));
	offset += sizeof(T);
	return value;
}

//! Format: uint32_t len; string chars
//! changes offset by the amount of read bytes
template<typename TString>
void readUnalignedString(TString& stringToFill, const char* buffer, uint32_t& offset){
	uint32_t size = readUnaligned<uint32_t>(buffer, offset);
	stringToFill = TString(&(buffer[offset]), size);
	offset += size;
}

//! Unaligned object read (class needs to have read(buffer, offset) method
//! changes offset by the amount of read bytes
template<typename TObject>
void readUnalignedObject(TObject& object, const char* buffer, uint32_t& offset){
	object.read(buffer, offset);
}

#ifndef MICROCONTROLLER_PLATFORM
//! Format: uint32_t #objects; objects
//! changes offset by the amount of read bytes
template<typename TObject>
void readUnalignedObjectVector(std::vector<TObject>& objects, const char* buffer, uint32_t& offset){
	uint32_t size = readUnaligned<uint32_t>(buffer, offset);
	objects = std::vector<TObject>(size);
	for(uint32_t i=0; i<size; i++){
		readUnalignedObject<TObject>(objects[i], buffer, offset);
	}
}
#endif

template<typename T>
void writeUnaligned(char* buffer, uint32_t& offset, T value){
	memcpy(&(buffer[offset]), &value, sizeof(T));
	offset += sizeof(T);
}

template <typename T, typename TIndex = uint32_t>
void writeLittleEndian(uint8_t* buffer, TIndex& offset, const T& value){
	for(uint8_t i=0; i<sizeof(T); i++){
		uint8_t shift = i << 3;//i*8
		buffer[offset] = static_cast<uint8_t>((value >> shift) & 0xFF);//extract ith byte
		offset++;
	}
}

template <typename T, typename TIndex = uint32_t>
T readLittleEndian(uint8_t* buffer, TIndex& offset){
	T res = 0;
	for(uint8_t i=0; i<sizeof(T); i++){
		uint8_t shift = i << 3;//i*8
		res = res | (static_cast<T>(buffer[offset]) << shift);
		offset++;
	}
	return res;
}

template <typename T, typename TIndex = uint32_t>
void writeBigEndian(uint8_t* buffer, TIndex& offset, const T& value){
	for(uint8_t i=0; i<sizeof(T); i++){
		uint8_t shift = (sizeof(T)-i-1) << 3;//*8
		buffer[offset] = static_cast<uint8_t>((value >> shift) & 0xFF);//extract ith byte
		offset++;
	}
}

template <typename T, typename TIndex = uint32_t>
T readBigEndian(uint8_t* buffer, TIndex& offset){
	T res = 0;
	for(uint8_t i=0; i<sizeof(T); i++){
		uint8_t shift = (sizeof(T)-i-1) << 3;//*8
		res = res | (static_cast<T>(buffer[offset]) << shift);
		offset++;
	}
	return res;
}

#endif
