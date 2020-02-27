#ifndef Duernion_H_INCLUDED
#define Duernion_H_INCLUDED

#include <cmath>
#include <type_traits>
#include <ostream>
#include <iostream>

#include "Matrix.h"

//! A Duernion is a representation of 2D orientations analog to 3D orientations in a Quaternion
template <typename T>
class Duernion{

	public:
	
	T x;
	T y;
	
	//! construct from angle (radians)
	Duernion(T angle){
		static_assert(std::is_floating_point<T>::value);
		T halfAngle = angle/2;
		x = cos(halfAngle);
		y = sin(halfAngle);
	}
	
	//! construct unit Duernion
	Duernion():x(1),y(0){}
	
	//! construct from content
	Duernion(T x, T y):x(x),y(y){}
	
	//! construct from forward vector
	Duernion(Vector2D<T> fwd):Duernion(atan2(fwd.get(1),fwd.get(0))){}
	
	//! return radians
	T calcAngle() const{
		return 2*(y>0?acos(x):acos(-x));
	}
	
	//! Theoretically a duernion is always normalized, but due to rounding errors it may be necessary to do it manually
	Duernion<T>& normalize(){
		T l = x*x+y*y;
		if(l!=1){
			l = sqrt(l);
			x /= l;
			y /= l;
		}
		return *this;
	}
	
	T dotProduct(const Duernion<T>& other) const{
		return x*other.x+y*other.y;
	}
	
	Duernion<T> operator+(const Duernion<T>& b) const{
		return Duernion<T>(x+b.x, y+b.y);
	}

	Duernion<T> operator*(T b) const{
		return Duernion<T>(x*b, y*b);
	}
	
	//! result equals the sequential execution of the two rotations (uses the complex number interpretation)
	Duernion<T> operator*(const Duernion<T>& b) const{
		return Duernion<T>(x*b.x-y*b.y, x*b.y+y*b.x);
	}
	
	//! multiplies with a 2D vector by reinterpreting the 2D vector as Duernion
	template<typename TMatrix>
	typename std::enable_if<TMatrix::isMatrix, Duernion<T>>::type operator*(const TMatrix& b){
		static_assert(TMatrix::columnCount==1 && TMatrix::rowCount==2);
		return *this * Duernion<T>(b.get(0), b.get(1));//Duernion<T>(x*b.get(0)-y*b.get(1), x*b.get(1)+y*b.get(0));
	}
	
	Duernion<T> getInverse(){
		return Duernion<T>(-x, y);
	}
	
	//! sets this duernion as a result of a lerp of two duernions, time specifices the progress of the interpolaton form start (0) to end (1)
	Duernion<T>& lerp(const Duernion<T>& start, const Duernion<T>& end, double time){
		return (*this = (start*(((T)1)-time)) + (end*time));
	}
	
	//! sets this duernion as a result of a slerp of two duernions, time specifices the progress of the interpolaton form start (0) to end (1), threshold is used to switch to lerp in case of small angles for numeric stability
	Duernion<T>& slerp(Duernion<T> start, const Duernion<T>& end, T time, T threshold = (T)0.05){
		T d = start.dotProduct(end);
		d = d>1?1:(d<-1?-1:d);//for robustness against rounding errors
		if(d<0){//same side for interpolation
			start.x = -start.x; start.y = -start.y;
			d = -d;
		}
		if(d<=(1-threshold)){
			const T theta = acos(d);
			const T invsintheta = ((T)1)/sin(theta);
			const T scale = sin(theta*(((T)1)-time))*invsintheta;
			const T invscale = sin(theta*time)*invsintheta;
			*this = (start*scale) + (end*invscale);
		}else{
			lerp(start,end,time);
		}
		normalize();//just in case of rounding errors
		return *this;
	}
	
	//! returns square (this*this);
	Duernion<T> sq() const{
		return Duernion<T>(x*x-y*y, 2*x*y);
	}
	
	Matrix<2,2,T> convertToMatrix() const{
		T cosAlpha = x*x-y*y;//Derivation: calculate this*this to double the contained half angle and use the euler identity (or look at the contructor ;-) )
		T sinAlpha = 2*x*y;
		return Matrix<2,2,T>{
			cosAlpha,	-sinAlpha,
			sinAlpha,	cosAlpha};
	}
	
	template<typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
	Vector2D<T> transform(const TMatrix& b){
		static_assert(TMatrix::columnCount==1 && TMatrix::rowCount==2);
		auto res = this->sq()*b;
		return Vector2D<T>(res.x, res.y);
	}
	
};

template <typename T>
std::ostream& operator<<(std::ostream& out, const Duernion<T>& d){
	return out << d.x << ", " << d.y;
}

typedef Duernion<double> DDuernion;
typedef Duernion<float> FDuernion;

#endif
