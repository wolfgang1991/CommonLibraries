#ifndef UCPCG32_H
#define UCPCG32_H

#include <stdint.h>

//! Fast, simple, and efficient 32-bit PCG random number generator
//! based on the C example provided by the PCG project by Melissa O'Neill
class UCPCG32{

    uint64_t state;
    uint64_t inc;

	void init(uint64_t initstate, uint64_t initseq){
		state = 0U;
		inc = (initseq << 1u) | 1u;
		next();
		state += initstate;
		next();
	}

	public:

	//! numbers can be anything, e.g. initialized by time (epoch seconds or milliseconds)
	UCPCG32(uint64_t initstate, uint64_t initseq){
		init(initstate, initseq);
	}

	//! init from MAC address
	UCPCG32(uint8_t* macAddress, uint8_t macLength = 6){
		uint64_t initstate = 0;
		for(uint8_t i = 0; i < macLength; i++){
			initstate = (initstate << 8) | macAddress[i];
		}
		init(initstate, initstate);
	}

	uint32_t next(){
		uint64_t oldstate = state;
		state = oldstate * 6364136223846793005ULL + inc;
		uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
		uint32_t rot = oldstate >> 59u;
		return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
	}
};

#endif // UCPCG32_H