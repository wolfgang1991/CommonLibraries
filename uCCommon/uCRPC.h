#ifndef UCRPC_H_
#define UCRPC_H_

#include "uCTiming.h"
#include "uCTypeTraits.h"

#include "ForEachMacro.h"
#include "BitFunctions.h"
#include "CRC32.h"
#include "platforms.h"
#include "uCArray.h"

#include <cinttypes>

#ifdef MICROCONTROLLER_PLATFORM
#define UCRPC_DEBUG(MSG)
#else
#include <misc.h>
#include <string>
#include <iostream>
#define UCRPC_DEBUG(MSG) std::cerr << MSG << std::endl;
#endif

namespace UCRPC{

	template <typename T>
	using is_bool = ucstd::is_same<T, bool>;

	template <typename T>
	struct is_integer{static constexpr bool value = ucstd::is_same<T, int8_t>::value || ucstd::is_same<T, int16_t>::value || ucstd::is_same<T, int32_t>::value || ucstd::is_same<T, int64_t>::value || ucstd::is_same<T, uint8_t>::value || ucstd::is_same<T, uint16_t>::value || ucstd::is_same<T, uint32_t>::value || ucstd::is_same<T, uint64_t>::value;};

	template <typename T>
	struct is_float{static constexpr bool value = ucstd::is_same<T, float>::value || ucstd::is_same<T, double>::value;};
	
	//! for integers/bools
	template<typename T, typename TIndex, typename ucstd::enable_if<is_bool<T>::value || is_integer<T>::value, int>::type = 0>
	void serialize(uint8_t* buffer, TIndex& offset, const T& value){
		writeLittleEndian(buffer, offset, value);
	}
	
	//! for objects
	template<typename T, typename TIndex, typename ucstd::enable_if<!is_bool<T>::value && !is_integer<T>::value && !is_float<T>::value && !ucstd::is_array<T>::value, int>::type = 0>
	void serialize(uint8_t* buffer, TIndex& offset, const T& value){
		offset += value.template serialize<TIndex>(&(buffer[offset]));
	}
	
	//! for arrays
	template<typename T, typename TIndex, typename ucstd::enable_if<ucstd::is_array<T>::value, int>::type = 0>
	void serialize(uint8_t* buffer, TIndex& offset, const T& value){
		for(size_t i=0; i<value.size(); i++){
			serialize(buffer, offset, value[i]);
		}
	}
	
	//! for integers/bools
	template<typename T, typename TIndex, typename ucstd::enable_if<is_bool<T>::value || is_integer<T>::value, int>::type = 0>
	void deserialize(const uint8_t* buffer, TIndex& offset, TIndex bufferSize, T& outValue){
		outValue = readLittleEndian<T,TIndex>(buffer, offset);
	}
	
	//! for objects,
	template<typename T, typename TIndex, typename ucstd::enable_if<!is_bool<T>::value && !is_integer<T>::value && !is_float<T>::value && !ucstd::is_array<T>::value, int>::type = 0>
	void deserialize(const uint8_t* buffer, TIndex& offset, TIndex bufferSize, T& outValue){
		outValue.deserialize(buffer, offset, bufferSize);
	}
	
	//! for arrays
	template<typename T, typename TIndex, typename ucstd::enable_if<ucstd::is_array<T>::value, int>::type = 0>
	void deserialize(const uint8_t* buffer, TIndex& offset, TIndex bufferSize, T& outValue){
		for(size_t i=0; i<outValue.size(); i++){
			deserialize(buffer, offset, bufferSize, outValue[i]);
		}
	}
	
	template<typename T>
	struct is_scalar{static constexpr bool value = is_bool<T>::value || is_integer<T>::value || is_float<T>::value;};
	
	//! for scalars
	template<typename T, typename TIndex, typename ucstd::enable_if<is_scalar<T>::value, int>::type = 0>
	constexpr TIndex getSpaceRequirement(){
		return sizeof(T);
	}
	
	//! for objects
	template<typename T, typename TIndex, typename ucstd::enable_if<!is_scalar<T>::value && !ucstd::is_array<T>::value, int>::type = 0>
	constexpr TIndex getSpaceRequirement(){
		return ucstd::remove_reference<T>::type::template getSpaceRequirement<TIndex>();
	}
	
	//! for arrays
	template<typename T, typename TIndex, typename ucstd::enable_if<ucstd::is_array<T>::value, int>::type = 0>
	constexpr TIndex getSpaceRequirement(){
		return T::size() * getSpaceRequirement<typename T::value_type, TIndex>();
	}
	
	//! syntactic sugar for deserialize usage on function return values, returns true if successful
	template<typename T, typename TIndex>
	bool deserializeResult(const uint8_t* buffer, TIndex bufferSize, T& outValue){
		bool ok = getSpaceRequirement<T,TIndex>()<=bufferSize;
		if(ok){
			TIndex offset = 0;
			deserialize(buffer, offset, bufferSize, outValue);
		}
		return ok;
	}
	
	//! A string representation for ucrpc, needs some special implementations for getSpaceRequirement, serialize and deserialize
	template <uint16_t maxLength, typename TChar = char>
	class String{
		
		public:
		
		uint16_t actualLength;
		TChar d[maxLength];
		
		static constexpr uint16_t capacity = maxLength;
		using value_type = TChar;
		
		String():actualLength(0){}
		
		String(const TChar* cString){
			actualLength = 0;
			operator<<(cString);
		}
		
		String(const TChar* cString, uint16_t cStringLength){
			setFromData(cString, cStringLength);
		}
		
		void setFromData(const TChar* cString, uint16_t cStringLength){
			if(cStringLength>maxLength){cStringLength = maxLength;}
			memcpy(d, cString, cStringLength);
			actualLength = cStringLength;
		}
		
		String(char c){
			d[0] = c;
			actualLength = 1;
		}
		
		#ifndef MICROCONTROLLER_PLATFORM
		String(const std::string& s){
			actualLength = maxLength>s.size()?s.size():maxLength;
			memcpy(d, &(s[0]), actualLength);
		}
		
		std::string convertToStdString() const{
			return std::string((char*)d, actualLength);
		}
		#endif
		
		uint16_t size() const{
			return actualLength;
		}
		
		bool empty() const{
			return actualLength==0;
		}
		
		bool full() const{
			return actualLength>=maxLength;
		}
		
		TChar* data(){
			return d;
		}
		
		const TChar* data() const{
			return d;
		}
		
		TChar operator[](uint16_t index) const{
			return d[index];
		}
		
		TChar& operator[](uint16_t index){
			return d[index];
		}
		
		static constexpr uint16_t getMaxLength(){
			return maxLength;
		}
		
		//! Warning no range check for single char
		void push_back(TChar b){
			d[actualLength] = b;
			actualLength++;
		}
		
		//! no initialization, must not be larger maxLength
		void resize(uint16_t newLength){
			actualLength = newLength;
		}
		
		String<maxLength, TChar>& operator<<(int32_t i){
			size_t len = maxLength-actualLength;
			int written = snprintf(&d[actualLength], len, "%" PRIi32, i);
			if(written>=0 && written<=len){
				actualLength += written;
			}
			return *this;
		}
		
		String<maxLength, TChar>& operator<<(const TChar* cString){
			uint16_t length = 0; for(;cString[length]!='\0';length++){}
			uint16_t toCopy = (maxLength>actualLength+length)?length:(maxLength-actualLength);
			memcpy(&(d[actualLength]), cString, toCopy);
			actualLength += toCopy;
			return *this;
		}
		
		void clear(){
			actualLength = 0;
		}
		
		template<typename TIndex>
		static constexpr TIndex getSpaceRequirement(){
			return sizeof(uint16_t)+maxLength;
		}
		
		template<typename TIndex>
		TIndex serialize(uint8_t* target) const{
			TIndex offset = 0;
			UCRPC::serialize<decltype(actualLength)>(target, offset, actualLength);
			memcpy(&(target[offset]), d, actualLength);
			return offset+actualLength;
		}
			
		template<typename TIndex>
		void deserialize(const uint8_t* buffer, TIndex& offset, TIndex bufferSize){
			UCRPC::deserialize<decltype(actualLength)>(buffer, offset, bufferSize, actualLength);
			if(actualLength>maxLength){actualLength = maxLength;}
			if(actualLength>bufferSize-offset){actualLength = bufferSize-offset;}
			memcpy(d, &(buffer[offset]), actualLength);
			offset += actualLength;
		}
		
	};
	
	//! A string interpretation of foreign memory with binary compatibility to String when sending via rpc (useful to save copy operations)
	//! maxLength is used to determine buffer sizes for rpc implementation
	template <uint16_t maxLength, typename TChar = char>
	class StringInterpretation{
		
		uint16_t actualLength;
		const TChar* d;
		
		public:
		
		static constexpr uint16_t capacity = maxLength;
		using value_type = TChar;
		
		StringInterpretation(){
			actualLength = 0;
		}
		
		StringInterpretation(const StringInterpretation& other){
			actualLength = other.actualLength;
			d = other.d;
		}
		
		StringInterpretation(const TChar* cString){
			d = cString;
			actualLength = strlen(cString);
		}
		
		StringInterpretation(const TChar* cString, uint16_t cStringLength){
			d = cString;
			actualLength = cStringLength;
		}
		
		uint16_t size() const{
			return actualLength;
		}
		
		bool empty() const{
			return actualLength==0;
		}
		
		const TChar* data() const{
			return d;
		}
		
		TChar operator[](uint16_t index) const{
			return d[index];
		}
		
		template<typename TIndex>
		static constexpr TIndex getSpaceRequirement(){
			return sizeof(uint16_t)+maxLength;
		}
		
		template<typename TIndex>
		TIndex serialize(uint8_t* target) const{
			TIndex offset = 0;
			UCRPC::serialize<decltype(actualLength)>(target, offset, actualLength);
			if(d!=NULL){memcpy(&(target[offset]), d, actualLength);}
			return offset+actualLength;
		}
			
		template<typename TIndex>
		void deserialize(const uint8_t* buffer, TIndex& offset, TIndex bufferSize){
			UCRPC::deserialize<decltype(actualLength)>(buffer, offset, bufferSize, actualLength);
			if(actualLength>maxLength){actualLength = maxLength;}
			if(actualLength>bufferSize-offset){actualLength = bufferSize-offset;}
			d = (const TChar*)&(buffer[offset]);
			offset += actualLength;
		}
		
	};
	
	//! Encode variable length arrays with a maximum capacity. Only sends and receives the used capacity (safety tradeoff: lengths cannot be checked, see UCRPC_DESERIALIZE_PARAMS_WITH_STRINGS)
	template <typename TObject, uint16_t maxLength>
	struct ObjectString{
		
		ucstd::array<TObject, maxLength> data;
		uint16_t actualLength;
		
		public:
		
		bool full() const{
			return actualLength>=maxLength;
		}
		
		void clear(){
			actualLength = 0;
		}
		
		const TObject& operator[](uint16_t index) const{
			return data[index];
		}
		
		TObject& operator[](uint16_t index){
			return data[index];
		}
		
		uint16_t size() const{
			return actualLength;
		}
		
		bool empty() const{
			return actualLength==0;
		}
		
		static constexpr uint16_t getMaxLength(){
			return maxLength;
		}
		
		//! Warning no range check for single object
		void push_back(const TObject& object){
			data[actualLength] = object;
			actualLength++;
		}
		
		//! no initialization, must not be larger maxLength
		void resize(uint16_t newLength){
			actualLength = newLength;
		}

		template<typename TIndex>
		static constexpr TIndex getSpaceRequirement(){
			return sizeof(actualLength)+UCRPC::getSpaceRequirement<decltype(data), TIndex>();
		}
		
		template<typename TIndex>
		TIndex serialize(uint8_t* target) const{
			TIndex offset = 0;
			UCRPC::serialize<decltype(actualLength)>(target, offset, actualLength);
			for(uint16_t i=0; i<actualLength; i++){
				UCRPC::serialize<decltype(data[i])>(target, offset, data[i]);
			}
			return offset;
		}
			
		template<typename TIndex>
		void deserialize(const uint8_t* buffer, TIndex& offset, TIndex bufferSize){
			UCRPC::deserialize<decltype(actualLength)>(buffer, offset, bufferSize, actualLength);
			if(actualLength>maxLength){actualLength = maxLength;}
			for(uint16_t i=0; i<actualLength; i++){
				UCRPC::deserialize<decltype(data[i])>(buffer, offset, bufferSize, data[i]);
			}
		}
		
	};
	
	//! special implementation for strings
	template<typename TIndex, uint16_t maxLength>
	bool deserializeResult(const uint8_t* buffer, TIndex bufferSize, String<maxLength>& outValue){
		TIndex offset = 0; deserialize(buffer, offset, bufferSize, outValue); // sanity checks included in deserialize
		return true;//always true / string may contain rubbish in case of malevolent sender
	}
	
	//! special implementation for strings
	template<typename TIndex, uint16_t maxLength>
	bool deserializeResult(const uint8_t* buffer, TIndex bufferSize, StringInterpretation<maxLength>& outValue){
		TIndex offset = 0; deserialize(buffer, offset, bufferSize, outValue); // sanity checks included in deserialize
		return true;//always true / string may contain rubbish in case of malevolent sender
	}
	
	//TODO serialize/deserialize/getSpaceRequirement: floats

	class IUCRemoteProcedureCaller{
		
		public:
		
		enum ProcedureError{
			NOT_FOUND,//! procedure not found at destination
			TIMEOUT,//! procedure could not be called in the given time (maybe no connection?)
			NO_FREE_FUNCTION_CALLS,//! procedure could not be called because too many calls are pending
			SENDBUFFER_TOO_SMALL,//! sendbuffer is too small for the given parameters
			BAD_FORMAT,//something is wrong regarding the binary format (e.g. lengths don't fit the expected length)
			ERROR_COUNT
		};
		
		virtual ~IUCRemoteProcedureCaller(){}
		
		virtual void OnProcedureResult(uint8_t* result, uint16_t resultLength, uint16_t functionID) = 0;
		
		virtual void OnProcedureError(ProcedureError error, uint16_t functionID) = 0;
		
	};
	
	//! Interface to provide a way to return values at runtime
	template <typename TIndex, TIndex maxParameterSize>
	class IUCRPC{
		
		public:
		
		virtual ~IUCRPC(){}
		
		virtual bool expectsReturnValue() const = 0;
		
		virtual void returnValue(uint8_t* returnValue, TIndex length) = 0;
		
		virtual void returnError(IUCRemoteProcedureCaller::ProcedureError error) = 0;
		
		template <typename TResult>
		void returnValue(const TResult& result){
			TIndex spaceReq = getSpaceRequirement<TResult, TIndex>();
			if(spaceReq<=maxParameterSize){
				uint8_t buffer[spaceReq];
				TIndex usedParameterSpace = 0;
				serialize(buffer, usedParameterSpace, result);
				returnValue(buffer, usedParameterSpace);
			}else{
				returnError(IUCRemoteProcedureCaller::SENDBUFFER_TOO_SMALL);
			}
		}
		
	};
	
	template <typename TIndex, TIndex maxParameterSize>
	class IUCRemoteProcedureCallReceiver{
		
		public:
		
		virtual ~IUCRemoteProcedureCallReceiver(){}
		
		virtual void callProcedure(IUCRPC<TIndex,maxParameterSize>* ucrpc, uint16_t functionID, uint8_t* parameter, uint16_t parameterLength) = 0;
		
	};
	
	template <typename TIndex, TIndex maxParameterSize>
	struct RegisteredFunction{
		uint16_t functionID;
		IUCRemoteProcedureCallReceiver<TIndex,maxParameterSize>* receiver;
		bool operator<(const RegisteredFunction& other) const{return functionID < other.functionID;}
	};
	
	static constexpr uint16_t getAdditionalRetryPeriod(uint16_t i){
		return 1 << (i%6);//0..32ms
	}

	//! A lightweight remote procedure call implementation for microcontrollers over serial communication
	//! TSerial must not block during receive or send. If data cannot be send they shall be discarded.
	//! maxParameterSize: max. amount of bytes of the largest parameters data (see callProcedure) and the largest return value
	//! minBufferFillToSend: how many bytes must be there before a message is sent (optimum for USB/VCP in case of a STM32 is 16-32 bytes)
	//! maxSimultaneousFunctionCalls: should be a small number to save space in case of microcontrollers, in case of a PC it may be large
	//! retryPeriod: time in ms after which a function call is retried (+ some offset to avoid simultanous sending)
	//! for compatibility between different API versions new functions have to be introduced, optional fields are not possible
	//! sendPeriod time in ms after which a package is sent regardless if the accumulated size is optimal
	//! Important: It can happen that a function call happens twice if the return value gets lost (automatic resend of remote procedure call). An API needs to be designed to have no problems with that.
	//! maxSendBytesBeforeReceive: (useful from host perspective) max amount of bytes which can be sent before a receive (excess bytes are discarded), 0 disables this feature. On some STM32 microcontrollers the USB receive buffer must not be fully filled (deadlock, freeze). In this case maxSendBytesBeforeReceive should be around half the USB receive buffer.
	//! maxSendReceiveTimeUS: time in us after which sending/receiving is paused to yield to other "tasks" (manual scheduling), there are two read tasks (without second serial one) and one send task in the update function  (worst case delay is 3*maxSendReceiveTimeUS)
	//! retryCount: if 0 retries calls forever
	template <typename TIndex, typename TSerial, TIndex maxParameterSize, uint8_t maxParallelFunctionCalls, typename TRegisteredFunctions, uint16_t maxSendBytesBeforeReceive = 0, typename TSecondarySerial = TSerial, uc_time_t initialRetryPeriod = 200, uint8_t initialRetryCount = 10, uint16_t minBufferFillToSend = 16, uc_time_t sendPeriod = 10, uc_time_t maxSendReceiveTimeUS = 10000/3>
	class UCRPC : public IUCRPC<TIndex, maxParameterSize>{

		public:
		
		//! stuffed package size with delimeter, function id, uid and escaped data
		static constexpr TIndex getMaxPackageSize(TIndex parameterSize){
			return 2+2*sizeof(uint16_t)+2*sizeof(uint8_t)+2*(parameterSize>1?parameterSize:1)+2*4;//delimeters + worst case doubling of function id, uid, parameterSize/errorCodeSize, crc32 checksum
		}
		
		static constexpr TIndex bufferSize = getMaxPackageSize(maxParameterSize);
		
		private:

		static constexpr uint8_t delimeter = 0b10101010;
		static constexpr uint8_t escape = 0b11001100;
		static constexpr uint16_t returnFlag = 1 << 15;
		static constexpr uint16_t errorFlag = 1 << 14;//13 bits remaining for actual function id
		
		TSerial& serial;
		TSecondarySerial* secondarySerial;
		
		uint8_t sendBuffer[bufferSize];
		TIndex sendBufferOffset;
		
		uint8_t rcvBuffer[bufferSize];
		TIndex rcvBufferOffset;
		
		uint8_t fwdBuffer[bufferSize];
		TIndex fwdBufferOffset;
		
		struct Call{
			uint16_t functionID;
			uint8_t uniqueID;//0: no unique id / result irrelevant
			IUCRemoteProcedureCaller* caller;
			uint8_t parameter[maxParameterSize];
			TIndex usedParameterSpace;
			uc_time_t lastSendTime;
			uint8_t sendCount;
		};
		
		Call calls[maxParallelFunctionCalls];
		Call* callPtrs[maxParallelFunctionCalls];//allows re-sorting without copying if function returns
		uint8_t callPtrIndex;//index of the next available call
		
		uint16_t bytesSentBeforeReceive;
		
		void erase(Call* c){
			for(uint8_t i=0; i<callPtrIndex; i++){
				if(callPtrs[i]==c){
					callPtrIndex--;
					Call* tmp = callPtrs[i];
					callPtrs[i] = callPtrs[callPtrIndex];
					callPtrs[callPtrIndex] = tmp;
					return;
				}
			}
		}
		
		Call* findCall(uint8_t uniqueID){
			for(uint8_t i=0; i<callPtrIndex; i++){
				if(callPtrs[i]->uniqueID==uniqueID){
					return callPtrs[i];
				}
			}
			return nullptr;
		}
		
		uint8_t uidCounter;
		
		uint8_t retryCount;
		uint32_t retryPeriod;
		
		uint8_t getUnusedUID(){
			bool used = true;
			while(used){
				uidCounter++;
				if(uidCounter==0){uidCounter++;}//don't use 0 => signals no return
				bool notFound = true;
				for(uint8_t i=0; notFound && i<callPtrIndex; i++){
					notFound = callPtrs[i]->uniqueID != uidCounter;
				}
				used = !notFound;
			}
			return uidCounter;
		}
		
		// returns false if erased
		bool resendCall(Call* c, uc_time_t t){
			if(c->sendCount<retryCount || retryCount==0){
				c->sendCount++;
				c->lastSendTime = t;
				if(!writeFunctionPackage(c->functionID, c->uniqueID, c->parameter, c->usedParameterSpace, true)){
					c->caller->OnProcedureError(IUCRemoteProcedureCaller::SENDBUFFER_TOO_SMALL, c->functionID);//should not be possible / should fail first
					erase(c);
					return false;
				}
				return true;
			}else{
				c->caller->OnProcedureError(IUCRemoteProcedureCaller::TIMEOUT, c->functionID);
				erase(c);
				return false;
			}
		}
		
		void writeDelimeter(){
			sendBuffer[sendBufferOffset] = delimeter;
			sendBufferOffset++;
		}
		
		//to sendbuffer, returns updated crc32
		uint32_t writeEscaped(uint8_t* data, TIndex length, uint32_t crc32){
			for(TIndex i=0; i<length; i++){
				uint8_t d = data[i];
				crc32 = UPDC32(d, crc32);
				if(d==delimeter || d==escape){
					sendBuffer[sendBufferOffset] = escape;
					sendBufferOffset++;
					sendBuffer[sendBufferOffset] = ~d;
					sendBufferOffset++;
				}else{
					sendBuffer[sendBufferOffset] = d;
					sendBufferOffset++;
				}
			}
			return crc32;
		}
		
		//to sendbuffer
		template <typename TScalar>
		uint32_t writeEscapedScalar(const TScalar& v, uint32_t crc32){
			uint8_t tmp[sizeof(v)]; uint8_t offset = 0;
			writeLittleEndian(tmp, offset, v);
			return writeEscaped(tmp, sizeof(v), crc32);
		}
		
		//! return true if sent / fitted into send buffer
		bool writeFunctionPackage(uint16_t functionID, uint8_t uid, uint8_t* params, TIndex length, bool forceSend){
			TIndex required = getMaxPackageSize(length);
			if(required>bufferSize){
				return false;
			}else if(required>bufferSize-sendBufferOffset){
				send(true);
			}
			writeDelimeter();
			STARTCRC32(crc32)
			crc32 = writeEscapedScalar<uint16_t>(functionID, crc32);
			crc32 = writeEscaped(&uid, 1, crc32);
			crc32 = writeEscaped(params, length, crc32);
			ENDCRC32(crc32)
			writeEscapedScalar<uint32_t>(crc32, 0);
			writeDelimeter();
			if(forceSend || sendBufferOffset>minBufferFillToSend){
				send(true);
			}
			return true;
		}
		
		void send(bool force = false){
			if(force || maxSendBytesBeforeReceive==0 || maxSendBytesBeforeReceive-bytesSentBeforeReceive>=sendBufferOffset){
				serial.send((const char*)sendBuffer, sendBufferOffset);
				bytesSentBeforeReceive += sendBufferOffset;//overflow ok, can only happen with maxSendBytesBeforeReceive==0
			}
			sendBufferOffset = 0;
		}
		
		//! returns new length, crc32: crc32 to update
		static TIndex unescapeInPlace(uint8_t* data, TIndex length){
			bool isEscaped = false;
			TIndex j = 0;
			for(TIndex i=0; i<length; i++){
				uint8_t c = data[i];
				if(isEscaped){
					isEscaped = false;
					data[j] = ~c;
					j++;
				}else{
					isEscaped = c==escape;
					if(!isEscaped){
						data[j] = c;
						j++;
					}
				}
			}
			return j;
		}
		
		static uint32_t updateCRC(uint8_t* data, TIndex length, uint32_t crc32){
			for(TIndex i=0; i<length; i++){
				crc32 = UPDC32(data[i], crc32);
			}
			return crc32;
		}
		
		//from buffer, crc32: crc32 to update
		template <typename TScalar>
		static TScalar readEscapedScalar(uint8_t* buffer, TIndex length, TIndex& offset, uint32_t& crc32){
			uint8_t tmp[sizeof(TScalar)];
			bool isEscaped = false;
			TIndex j=0;
			for(; offset<length && j<sizeof(TScalar); offset++){
				uint8_t c = buffer[offset];
				if(isEscaped){
					isEscaped = false;
					tmp[j] = ~c;
					crc32 = UPDC32(tmp[j], crc32);
					j++;
				}else{
					isEscaped = c==escape;
					if(!isEscaped){
						tmp[j] = c;
						crc32 = UPDC32(c, crc32);
						j++;
					}
				}
			}
			uint8_t o = 0;
			return readLittleEndian<TScalar>(tmp, o);
		}
		
		TRegisteredFunctions& functions;
		
		bool hasLastFunction;
		uint16_t lastFunctionID;
		uint8_t lastUid;//! 0 means no response required
		uc_time_t lastSendTime;
		
		public:
		
		using MappedIndex = TIndex;
		static constexpr TIndex MappedMaxParameterSize = maxParameterSize;
		
		UCRPC(TSerial& serial, TRegisteredFunctions& functions):serial(serial),secondarySerial(NULL),sendBufferOffset(0),rcvBufferOffset(0),fwdBufferOffset(0),callPtrIndex(0),bytesSentBeforeReceive(0),uidCounter(0),functions(functions),hasLastFunction(false),lastSendTime(0){
			retryCount = initialRetryCount;
			retryPeriod = initialRetryPeriod;
			functions.sort();
			for(uint8_t i=0; i<maxParallelFunctionCalls; i++){
				callPtrs[i] = &(calls[i]);
			}
		}
		
		//! 0: retries calls forever
		//! retryPeriod: used for timeout (fail) and resend
		void setRetryParameters(uint8_t retryCount = initialRetryCount, uc_time_t retryPeriod = initialRetryPeriod){
			this->retryCount = retryCount;
			this->retryPeriod = retryPeriod;
		}
		
		uint8_t getRetryCount() const{
			return retryCount;
		}
		
		uc_time_t getRetryPeriod() const{
			return retryPeriod;
		}
		
		void setSecondarySerial(TSecondarySerial* s){
			secondarySerial = s;
		}
		
		//! TParameter represents the function parameters and is either a scalar or needs to have the serialize, deserialize and static constexpr getSpaceRequirement functions
		//! If caller is present: Reception or error callback calling is guaranteed.
		//! If no caller is present: It get's sent if it fits into the sendbuffer regardless of free function calls.
		template <typename TParameter>
		void callRemoteProcedure(uint16_t functionID, const TParameter& parameter, IUCRemoteProcedureCaller* caller = nullptr){
			constexpr TIndex spaceReq = getSpaceRequirement<TParameter, TIndex>();
			static_assert(spaceReq<=maxParameterSize, "Sendbuffer too small");
			if(caller){
				if(callPtrIndex<maxParallelFunctionCalls){
					Call* toFill = callPtrs[callPtrIndex];
					callPtrIndex++;
					toFill->functionID = functionID;
					toFill->uniqueID = getUnusedUID();
					toFill->caller = caller;
					toFill->usedParameterSpace = 0;
					serialize(toFill->parameter, toFill->usedParameterSpace, parameter);
					toFill->sendCount = 0;
					resendCall(toFill, millis());
				}else{
					caller->OnProcedureError(IUCRemoteProcedureCaller::NO_FREE_FUNCTION_CALLS, functionID);
				}
			}else{//no caller
				uint8_t buffer[spaceReq];
				TIndex usedParameterSpace = 0;
				serialize(buffer, usedParameterSpace, parameter);
				callRemoteProcedure(functionID, buffer, usedParameterSpace);
			}
		}
		
		//! like the other callRemoteProcedure but with a simple data array instead of parameters
		//! returns if successfully added for sending
		bool callRemoteProcedure(uint16_t functionID, uint8_t* data, TIndex length){
			return writeFunctionPackage(functionID, 0, data, length, false);
		}
		
		//! with raw data but with caller for return values / error handling
		void callRemoteProcedure(uint16_t functionID, uint8_t* data, TIndex length, IUCRemoteProcedureCaller* caller){
			if(callPtrIndex<maxParallelFunctionCalls){
				if(length<=maxParameterSize){
					Call* toFill = callPtrs[callPtrIndex];
					callPtrIndex++;
					toFill->functionID = functionID;
					toFill->uniqueID = getUnusedUID();
					toFill->caller = caller;
					toFill->usedParameterSpace = 0;
					memcpy(toFill->parameter, data, length);
					toFill->usedParameterSpace = length;
					toFill->sendCount = 0;
					resendCall(toFill, millis());
				}else{
					caller->OnProcedureError(IUCRemoteProcedureCaller::SENDBUFFER_TOO_SMALL, functionID);
				}
			}else{
				caller->OnProcedureError(IUCRemoteProcedureCaller::NO_FREE_FUNCTION_CALLS, functionID);
			}
		}
		
		//! gets called by IUCRemoteProcedureCallReceiver::callProcedure to return a value of the currently called function
		//! uid and function id are remembered from the current call
		void returnValue(uint8_t* returnValue, TIndex length) override{
			if(hasLastFunction && lastUid!=0){
				if(!writeFunctionPackage(lastFunctionID | returnFlag, lastUid, returnValue, length, false)){
					returnError(IUCRemoteProcedureCaller::SENDBUFFER_TOO_SMALL);
				}
				hasLastFunction = false;
			}
		}
		
		//! true if the current / last call expects a return value, useful from callProcedure
		bool expectsReturnValue() const{
			return hasLastFunction && lastUid!=0;
		}
		
		void returnError(IUCRemoteProcedureCaller::ProcedureError error) override{
			if(expectsReturnValue()){
				writeDelimeter();
				STARTCRC32(crc32)
				crc32 = writeEscapedScalar<uint16_t>(lastFunctionID | errorFlag, crc32);
				crc32 = writeEscaped(&lastUid, 1, crc32);
				uint8_t errorCode = (uint8_t)error;
				crc32 = writeEscaped(&errorCode, 1, crc32);
				ENDCRC32(crc32)
				writeEscapedScalar<uint32_t>(crc32, 0);
				writeDelimeter();
				if(sendBufferOffset>minBufferFillToSend){
					send();
				}
				hasLastFunction = false;
			}
		}
		
		//! can be used to check if a function call would fail
		bool hasFreeFunctionCalls() const{
			return callPtrIndex<maxParallelFunctionCalls;
		}
		
		void flush(){
			if(sendBufferOffset>0){send();}
		}
		
		//! returns true if new data has been received
		bool update(){
			//forward from secondary serial to serial
			if(secondarySerial){
				int32_t received = secondarySerial->recv((char*)&(fwdBuffer[fwdBufferOffset]), bufferSize-fwdBufferOffset);
				uc_time_t startT = micros();
				while(received>0){
					fwdBufferOffset += received;
					int32_t dataStart = -1, dataEnd = -1;
					uint8_t state = 0;//0: start before packet, 1: first packet delimeter read, 2: reading content
					for(TIndex i=0; i<fwdBufferOffset; i++){
						uint8_t c = fwdBuffer[i];
						if(state==0){
							if(c==delimeter){
								state = 1;
								if(dataStart<0){dataStart = i;}
							}
						}else if(state==1){
							if(c!=delimeter){state = 2;}
						}else{//state==2
							if(c==delimeter){
								dataEnd = i+1;//points one after the delimeter
								state = 0;
							}
						}		
					}
					if(dataStart>=0 && dataEnd>=0){//found something to forward
						serial.send((char*)&(fwdBuffer[dataStart]), dataEnd-dataStart);
						TIndex remaining = fwdBufferOffset-dataEnd;
						if(remaining>0){memmove(fwdBuffer, &(fwdBuffer[dataEnd]), remaining);}
						fwdBufferOffset = remaining;
					}else if(fwdBufferOffset==bufferSize || dataStart==-1){//buffer full or no start found => discard
						fwdBufferOffset = 0;
					}
					if(calcTimeDifference(micros(), startT)<maxSendReceiveTimeUS){
						received = secondarySerial->recv((char*)&(fwdBuffer[fwdBufferOffset]), bufferSize-fwdBufferOffset);
					}else{
						break;
					}
				}
//				if(received<0){
//					UCRPC_DEBUG("Error while receiving data from secondarySerial")
//				}
			}
			//Receive:
			int32_t received = serial.recv((char*)&(rcvBuffer[rcvBufferOffset]), bufferSize-rcvBufferOffset);
			if(received>0){bytesSentBeforeReceive = 0;}
			uc_time_t startT = micros();
			while(received>0){
				rcvBufferOffset += received;
				bool pkgMaybeAvail = true;
				TIndex pkgStartPos = 0;
				while(pkgMaybeAvail){
					for(; pkgStartPos<rcvBufferOffset && rcvBuffer[pkgStartPos]!=delimeter; pkgStartPos++){}
					if(pkgStartPos==rcvBufferOffset){//only rubbish
						rcvBufferOffset = 0;
						pkgMaybeAvail = false;
					}else{
						TIndex pkgEndPos = pkgStartPos + 1; for(; pkgEndPos<rcvBufferOffset && rcvBuffer[pkgEndPos]!=delimeter; pkgEndPos++){}
						if(pkgEndPos==rcvBufferOffset){//no end pos
							pkgMaybeAvail = false;
							if(pkgStartPos>0){//move package to beginning / overwrite rubbish
								memmove(rcvBuffer, &(rcvBuffer[pkgStartPos]), rcvBufferOffset-pkgStartPos);
								rcvBufferOffset = rcvBufferOffset-pkgStartPos;
								pkgStartPos = 0;
							}else if(rcvBufferOffset==bufferSize){//buffer full => discard
								rcvBufferOffset = 0;
							}
						}else if(pkgEndPos==pkgStartPos+1){//duplicate delimeter => border of packages
							pkgStartPos = pkgEndPos;
						}else{//found
							handlePackage(&(rcvBuffer[pkgStartPos]), pkgEndPos+1-pkgStartPos);
							pkgStartPos = pkgEndPos + 1;
						}
					}
				}
				if(calcTimeDifference(micros(), startT)<maxSendReceiveTimeUS){
					received = serial.recv((char*)&(rcvBuffer[rcvBufferOffset]), bufferSize-rcvBufferOffset);
				}else{
					break;
				}
			}
//			if(received<0){
//				UCRPC_DEBUG("Error while receiving data.")
//			}
			//Send:
			uc_time_t t = millis();
			startT = micros();
			//resend calls if necessary:
			uint8_t i=0;
			while(i<callPtrIndex && calcTimeDifference(micros(), startT)<maxSendReceiveTimeUS){//for(uc_time_t startT = micros(); i<callPtrIndex && calcTimeDifference(micros(), startT)<maxSendReceiveTimeUS;){//
				Call* c = callPtrs[i];
				if(calcTimeDifference(t, c->lastSendTime)>=retryPeriod+getAdditionalRetryPeriod(i)){
					if(resendCall(c, t)){i++;}
				}else{
					i++;
				}
			} 
			//send buffer if optimal or necessary:
			if(sendBufferOffset>minBufferFillToSend || (sendBufferOffset>0 && calcTimeDifference(t,lastSendTime)>sendPeriod)){
				send();
				lastSendTime = t;
			}
			return received>0;
		}
		
		//with delimeters && escaped
		void handlePackage(uint8_t* buffer, TIndex length){
			if(length>=9){//delimeter + function id + uid + crc32 + delimeter
				TIndex offset = 1;
				STARTCRC32(crc32)
				lastFunctionID = readEscapedScalar<uint16_t>(buffer, length, offset, crc32);//if length doesn't match it's rubbish (which is ok)
				if(offset>length-6){return;}//at least uid + crc32 + delimeter remaining
				lastUid = readEscapedScalar<uint8_t>(buffer, length, offset, crc32);
				if(offset>length-5){return;}//at least crc32 + delimeter remaining
				TIndex newLength = unescapeInPlace(&(buffer[offset]), length-1-offset);//-1 for last delimeter
				if(newLength>=4){//must at least hold crc
					crc32 = updateCRC(&(buffer[offset]), newLength-4, crc32);
					ENDCRC32(crc32)
					TIndex cpos = newLength-4; uint32_t readCRC = readLittleEndian<uint32_t>(&(buffer[offset]), cpos);
					if(readCRC==crc32){
						if((lastFunctionID & returnFlag) != 0){//return value read
							Call* c = findCall(lastUid);
							if(c){
								c->caller->OnProcedureResult(&(buffer[offset]), newLength-4, lastFunctionID & ~returnFlag);
								erase(c);
							}
						}else if((lastFunctionID & errorFlag) != 0){//return error read
							Call* c = findCall(lastUid);
							if(c && newLength>=5){//crc+error
								uint8_t error = buffer[offset];
								c->caller->OnProcedureError((IUCRemoteProcedureCaller::ProcedureError)error, lastFunctionID & ~errorFlag);
								erase(c);
							}else{
								UCRPC_DEBUG((c==NULL?"uid not found":"Error code missing in error package."))
							}
						}else{//function call read
							hasLastFunction = true;
							size_t fidx = functions.findBisect(RegisteredFunction<TIndex,maxParameterSize>{lastFunctionID});
							if(fidx==functions.size()){//not found
								if(secondarySerial){//forward package if forwarding method available else send error
									secondarySerial->send((char*)buffer, length);//send buffer (see HardwareSerial.h) must be large enough to hold the largest package
								}else{
									returnError(IUCRemoteProcedureCaller::NOT_FOUND);
								}
							}else{//found
								functions[fidx].receiver->callProcedure(this, lastFunctionID, &(buffer[offset]), newLength-4);
							}
						}
					}else{
						UCRPC_DEBUG("Bad CRC:")
						UCRPC_DEBUG(lastFunctionID)
					}
				}else{
					UCRPC_DEBUG("Bad Payload Length.")
				}
			}
		}


	};
	
}

#define UCRPC_SERIALIZE_START\
	template<typename TIndex>\
	TIndex serialize(uint8_t* ucrcp_macro_prefix_target) const{\
		TIndex ucrcp_macro_prefix_offset = 0;

#define UCRPC_WRITE(PARAM)\
	UCRPC::serialize<decltype(PARAM)>(ucrcp_macro_prefix_target, ucrcp_macro_prefix_offset, PARAM);
	
#define UCRPC_SERIALIZE_END\
		return ucrcp_macro_prefix_offset;\
	}

#define UCRPC_SPACEREQ_START\
	template<typename TIndex>\
	static constexpr TIndex getSpaceRequirement(){\
		return (TIndex)0

#define UCRPC_ADD_SPACEREQ(PARAM)\
		 + UCRPC::getSpaceRequirement<decltype(PARAM), TIndex>()

#define UCRPC_SPACEREQ_END\
		;\
	}

#define UCRPC_DESERIALIZE_START\
	template<typename TIndex>\
	void deserialize(const uint8_t* ucrcp_macro_prefix_buffer, TIndex& ucrcp_macro_prefix_offset, TIndex ucrcp_macro_prefix_bufferSize){

#define UCRPC_DESERIALIZE(PARAM)\
		UCRPC::deserialize<decltype(PARAM)>(ucrcp_macro_prefix_buffer, ucrcp_macro_prefix_offset, ucrcp_macro_prefix_bufferSize, PARAM);

#define UCRPC_DESERIALIZE_END\
	}

#define UCRPC_FUNCTIONS(...)\
	UCRPC_SERIALIZE_START\
	FOR_EACH(UCRPC_WRITE, __VA_ARGS__)\
	UCRPC_SERIALIZE_END\
	UCRPC_DESERIALIZE_START\
	FOR_EACH(UCRPC_DESERIALIZE,  __VA_ARGS__)\
	UCRPC_DESERIALIZE_END\
	UCRPC_SPACEREQ_START\
	FOR_EACH(UCRPC_ADD_SPACEREQ,  __VA_ARGS__)\
	UCRPC_SPACEREQ_END

template <typename TA, typename TB>
static constexpr TA uCRPCMax(TA a, TB b){
	return a>b?a:b;
}

#define UCRPC_MAX_SPACEREQ_START(CLASS)\
	uCRPCMax(UCRPC::getSpaceRequirement<CLASS,uint32_t>(), 

#define UCRPC_MAX_SPACEREQ_END(CLASS)\
	)

#define GET_MAX_SPACE_REQUIREMENT(...)\
	FOR_EACH(UCRPC_MAX_SPACEREQ_START,  __VA_ARGS__)\
	(uint32_t)0\
	FOR_EACH(UCRPC_MAX_SPACEREQ_END,  __VA_ARGS__)

//! assumes variable names like in IUCRemoteProcedureCallReceiver::callProcedure, handles also return errors
#define UCRPC_DESERIALIZE_PARAMS(VALUE, TINDEX)\
	if(UCRPC::getSpaceRequirement<decltype(VALUE),TINDEX>()>parameterLength){\
		ucrpc->returnError(UCRPC::IUCRemoteProcedureCaller::BAD_FORMAT);\
		return;\
	}\
	TINDEX ucrcp_macro_prefix_offset = 0;\
	UCRPC::deserialize(parameter, ucrcp_macro_prefix_offset, parameterLength, VALUE);

//! Dangerous in case of a malevolent/buggy sender, otherwise ok because of CRC
#define UCRPC_DESERIALIZE_PARAMS_WITH_STRINGS(VALUE, TINDEX)\
	TINDEX ucrcp_macro_prefix_offset = 0;\
	UCRPC::deserialize(parameter, ucrcp_macro_prefix_offset, parameterLength, VALUE);

struct UCRPC_EMTPY{
	uint8_t c;
	UCRPC_EMTPY():c(0){}
	UCRPC_FUNCTIONS(c)
};

#ifndef MICROCONTROLLER_PLATFORM

template <typename TResult, typename TRPC>
bool executeBlockingRPC(TResult& returnValue, TRPC* rpc, uint16_t functionID, const std::function<void(TRPC*,UCRPC::IUCRemoteProcedureCaller*)>& call){
	class : public UCRPC::IUCRemoteProcedureCaller{
		public:
		bool running = true;
		bool success = false;
		TResult* returnValue;
		uint16_t funcID;
		void OnProcedureResult(uint8_t* result, uint16_t resultLength, uint16_t functionID){
			if(functionID==funcID){
				running = false;
				success = UCRPC::deserializeResult(result, resultLength, *returnValue);
			}
		}
		void OnProcedureError(UCRPC::IUCRemoteProcedureCaller::ProcedureError error, uint16_t functionID){
			if(functionID==funcID){
				std::cout << "Procedure error of function " << functionID << ": " << (int)error << std::endl;
				running = success = false;
			}
		}
	} caller;
	caller.returnValue = &returnValue;
	caller.funcID = functionID;
	call(rpc, &caller);
	while(caller.running){
		rpc->update();
		delay(1);
	}
	return caller.success;
}

//! returns true if the rpc function returns and the expected value matches
template <typename TResult, typename TRPC>
bool executeBlockingRPCCheck(const TResult& expectValue, TRPC* rpc, uint16_t functionID, const std::function<void(TRPC*,UCRPC::IUCRemoteProcedureCaller*)>& call){
	TResult returnValue;
	if(executeBlockingRPC(returnValue, rpc, functionID, call)){
		return returnValue==expectValue;
	}
	return false;
}

#endif

#endif
