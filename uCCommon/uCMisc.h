#ifndef uCMisc_H_
#define uCMisc_H_

#include <stdint.h>

#undef min
#undef max
#undef abs

//! round fraction (numerator/denominator) without using floating point numbers
template<typename T>
T roundFraction(T numerator, T denominator){
	bool negative = (numerator<0 && denominator>0) || (numerator>=0 && denominator<0);
	if(numerator<0){numerator = -numerator;}
	if(denominator<0){denominator = -denominator;}
	return (negative?-1:1) * (numerator/denominator + (((T)2)*(numerator%denominator)>=denominator));
}

//! roundFraction (numerator/denominator) for unsigned types (more efficient)
template<typename T>
T roundFractionUnsigned(T numerator, T denominator){
	return numerator/denominator + (((T)2)*(numerator%denominator)>=denominator);
}

constexpr uint8_t CRC8_POLY = 0x07; // CRC-8 (poly x^8 + x^2 + x + 1)

inline uint8_t crc8_update(uint8_t crc, uint8_t data){
	// MSB-first bitwise implementation
	crc ^= data;
	for(uint8_t i = 0; i < 8; ++i){
		if(crc & 0x80){
			crc = static_cast<uint8_t>((crc << 1) ^ CRC8_POLY);
		}else{
			crc = static_cast<uint8_t>(crc << 1);
		}
	}
	return crc;
}

inline uint8_t crc8(const uint8_t* data, uint32_t length, uint8_t init = 0x00){
	uint8_t crc = init;
	for(uint32_t i = 0; i < length; ++i){
		crc = crc8_update(crc, data[i]);
	}
	return crc;
}

namespace ucstd{

	template<typename T>
	T clamp(T value, T min, T max){
		return value<min?min:(value>max?max:value);
	}
	
	template<typename T>
	T max(T a, T b){
		return a>b?a:b;
	}
	
	template<typename T>
	T min(T a, T b){
		return a<b?a:b;
	}
	
	template<typename T>
	T abs(T v){
		return v<(T)0?-v:v;
	}
	
	//! round floating point number A to integer B
	template <typename A, typename B>
	B rd(A in){
		bool sign = in<(A)0.0;
		in = sign?-in:in;//abs
		B out = (B)(in+(A)0.5);
		return sign?-out:out;
	}

	//! compile time log2 of unsigned integer n (rounded up)
	template <typename TUInt>
	constexpr TUInt log2(TUInt n){
		return (n<2?1:(1+log2(n/2)));
	}

	//! compile time power of 2
	template <typename TUInt>
	constexpr TUInt pow2(TUInt exp){
		return (exp==0?1:(2*pow2(exp-1)));
	}

};

#endif
