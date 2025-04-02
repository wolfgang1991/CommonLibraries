#ifndef RingBuffer_H_
#define RingBuffer_H_

template <uint32_t capacity>
class RingBuffer{//TODO more efficiency
	
	uint8_t data[capacity];
	uint32_t start, end;
	
	public:
	
	RingBuffer(){
		start = end = 0;
	}
	
	bool empty() const{
		return start==end;
	}
	
	uint32_t size(){
		if(start<end){
			return end-start;
		}else{
			return capacity-start+end;
		}
	}
	
	void push_back(uint8_t byte){
		data[end] = byte;
		end = (end+1)%capacity;
	}
	
	void write_back(const uint8_t* buf, uint32_t buflen){
		for(uint32_t i=0; i<buflen; i++){
			push_back(buf[i]);
			if(start==end){start = (start+1)%capacity;}
		}
	}
	
	uint8_t pop_front(){
		uint8_t v = data[start];
		start = (start+1)%capacity;
		return v;
	}
	
	//! returns the amount of bytes read
	uint32_t read_front(uint8_t* toFill, uint32_t fillAmount){
		uint32_t i=0; 
		for(; i<fillAmount && !empty(); i++){
			toFill[i] = pop_front();
		}
		return i;
	}
	
};

#endif
