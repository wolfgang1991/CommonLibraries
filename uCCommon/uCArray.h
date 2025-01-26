#ifndef uCArray_H_INCLUDED
#define uCArray_H_INCLUDED

#include "uCTypeTraits.h"

#include <stddef.h>

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
		//! returns next greater index instead of exact find or size() if findNextGreater true
		template<typename TFindValue>
		size_t findBisect(const TFindValue& toFind, bool findNextGreater = false) const{
			size_t a = 0, b = N;
			while(a!=b){
				size_t center = (a+b)/2;//div
				if(content[center]<toFind){
					if(a==center){if(findNextGreater){return center+1;}else{return N;}}//no change
					a = center;
				}else if(toFind<content[center]){
					if(b==center){if(findNextGreater){return center;}else{return N;}}//no change
					b = center;
				}else{//content[center]==toFind
					return center;
				}
			}
			if(findNextGreater){return a==N?N:(content[a]<toFind?(a+1):a);}else{return N;}
		}
		
		bool operator==(const array<T,N>& other) const{
			bool equal = true;
			for(size_t i=0; equal && i<N; i++){
				equal = content[i]==other[i];
			}
			return equal;
		}
		
	};
	
	template<typename T>
	struct is_array : ucstd::false_type {};

	template<typename T, size_t N>
	struct is_array<ucstd::array<T, N>> : ucstd::true_type {};
	
};

#endif
