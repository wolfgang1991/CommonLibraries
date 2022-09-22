#ifndef NMEA_HELPERS_H_INCLUDED
#define NMEA_HELPERS_H_INCLUDED

#include <stdint.h>
#include <string.h>

//#include <stdio.h>

template<typename T, typename TSize, bool enableReallocation>
struct OutBuffer{
	
	T* buffer;
	TSize size;
	TSize offset;
	
	OutBuffer(TSize initSize){
		buffer = initSize>0?(new T[initSize]):NULL;
		offset = 0;
		size = initSize;
		for(TSize i=0; i<size; i++){
			buffer[i] = 0;
		}
	}
	
	~OutBuffer(){
		if(buffer){delete[] buffer;}
	}
	
	void add(T value){
		if(offset>=size){
			if(!enableReallocation && buffer!=NULL){return;}
			TSize newSize = (size+1)*2;
			//printf("reallocate: %i\n", newSize);
			T* newBuffer = new T[newSize];
			if(buffer){
				memcpy(newBuffer, buffer, size);
				delete[] buffer;
			}
			buffer = newBuffer;
			size = newSize;
		}
		buffer[offset] = value;
		offset++;
	}
	
	void reset(){
		offset = 0;
	}
		
};

enum class PatternParseResult{
	RESET,//! buffer has been reset because no matching chars have been read
	PARSING,//! Parsing continues (matching chars)
	VALUE_READ,//! value read
	FINISHED//! pattern parsing finished (depending on the pattern a last value might be available)
};

//PatternSyntax: \ escape char, . any char, * any chars, ? value to parse, e.g.: "$..GGA,*,*,*,*,*,*,?,*\n";
//The char after \,*,? is interpreted literally.
//state == index in pattern
template<typename T, typename TSize, bool enableReallocation>
PatternParseResult parsePattern(char c, uint16_t& state, const T* pattern, OutBuffer<T, TSize, enableReallocation>& buffer){
	T patternChar = pattern[state];
	//printf("c: %c   patternChar[%i]: %c\n", c, state, patternChar);
	if(patternChar=='\\'){//Escape in Pattern
		if(pattern[state+1]==c){
			state += 2;
		}else{
			state = 0;
			buffer.reset();
			return PatternParseResult::RESET;
		}
	}else if(patternChar=='*'){//Any sequence of chars
		if(pattern[state+1]==c){
			state += 2;
		}
	}else if(patternChar=='?'){//Value
		char nextChar = pattern[state+1];
		bool nextEscape = nextChar=='\\';
		if(nextEscape){nextChar = pattern[state+2];}
		if(nextChar==c){
			buffer.add('\0');
			buffer.reset();
			if(c=='\0'){state = 0; return PatternParseResult::FINISHED;}
			state += nextEscape?3:2;
			if(pattern[state]=='\0'){state = 0; return PatternParseResult::FINISHED;}
			return PatternParseResult::VALUE_READ;
		}else{
			buffer.add(c);
		}
	}else if(patternChar=='.' || patternChar==c){//Any char or match
		state++;
	}else{
		state = 0;
		buffer.reset();
		return PatternParseResult::RESET;
	}
	if(pattern[state]=='\0'){//Finished
		state = 0;
		buffer.add('\0');
		buffer.reset();
		return PatternParseResult::FINISHED;
	}
	return PatternParseResult::PARSING;
}



#endif
