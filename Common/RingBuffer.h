#ifndef RingBuffer_H_
#define RingBuffer_H_

template <uint32_t capacity, typename T = uint8_t>
class RingBuffer{//TODO more efficiency
	
	T data[capacity];
	uint32_t start, end;
	
	public:
	
	RingBuffer(){
		start = end = 0;
	}
	
	bool empty() const{
		return start==end;
	}
	
	bool full() const{
		return (end+1)%capacity == start;
	}
	
	uint32_t size() const{
		if(end>=start){
			return end-start;
		}else{
			return capacity-start + end;
		}
	}
	
	void push_back(T byte){
		data[end] = byte;
		end = (end+1)%capacity;
		if(start==end){start = (start+1)%capacity;}
	}
	
	void write_back(const T* buf, uint32_t buflen){
		for(uint32_t i=0; i<buflen; i++){
			push_back(buf[i]);
		}
	}
	
	T back() const{
		return data[end];
	}
	
	T front() const{
		return data[start];
	}
	
	T pop_front(){
		T v = data[start];
		start = (start+1)%capacity;
		return v;
	}
	
	//! returns the amount of bytes read
	uint32_t read_front(T* toFill, uint32_t fillAmount){
		uint32_t i=0; 
		for(; i<fillAmount && !empty(); i++){
			toFill[i] = pop_front();
		}
		return i;
	}
	
};

#endif
