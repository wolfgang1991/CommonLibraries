#ifndef uCMisc_H_
#define uCMisc_H_

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

};

#endif
