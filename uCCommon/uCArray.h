#ifndef uCArray_H_INCLUDED
#define uCArray_H_INCLUDED

namespace ucstd{
	
	//! like std::array (some things missing)
	template <typename T, size_t N>
	struct array{
		
		using value_type = T;
		
		T content[N];
		
		template <typename... TInit> 
		array(TInit... ts):content{static_cast<T>(ts)...} {}
		
		static constexpr size_t size(){
			return N;
		}
		
		static constexpr bool empty(){
			return N==0;
		}
		
		T& operator[](size_t index){
			return content[index];
		}
		
		const T& operator[](size_t index) const{
			return content[index];
		}
		
		T* data(){
			return content;
		}
		
		const T* data() const{
			return content;
		}
		
		//! O(n^2), uses operator< TODO better algorithm
		void sort(){
			for(size_t i=0; i<N; i++){
				for(size_t j=i+1; j<N; j++){
					if(content[j]<content[i]){
						T tmp = content[i];
						content[i] = content[j];
						content[j] = tmp;
					}
				}
			}
		}
		
		//! on sorted arrays, returns size() if not found, uses operator<
		template<typename TFindValue>
		size_t findBisect(const TFindValue& toFind){
			size_t a = 0, b = N;
			while(a!=b){
				size_t center = (a+b)/2;//div
				if(content[center]<toFind){
					if(a==center){break;}//no change
					a = center;
				}else if(toFind<content[center]){
					if(b==center){break;}//no change
					b = center;
				}else{//content[center]==toFind
					return center;
				}
			}
			return size();
		}
		
		bool operator==(const array<T,N>& other) const{
			bool equal = true;
			for(size_t i=0; equal && i<N; i++){
				equal = content[i]==other[i];
			}
			return equal;
		}
		
	};
	
};

#endif
