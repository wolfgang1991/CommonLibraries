#ifndef MATHUTILS_H_INCLUDED
#define MATHUTILS_H_INCLUDED

#include <rect.h>
#include <vector2d.h>
#include <vector3d.h>

#include <cmath>

#define DEG_RAD (180.0/3.141592654)
#define DOUBLE_EPS 0.00000001
#define FLOAT_EPS 0.0001

template <typename A, typename B, typename C>
A Clamp(A value, B min, C max){
	return value<(A)min?(A)min:(value>(A)max?(A)max:value);
}

template <class T>
T Min(T value, T max){
	return value>max?max:value;
}

template <class T>
T Max(T value, T min){
	return value<min?min:value;
}

//! ^2
template <class T>
T sq(T val){
	return val*val;
}

//! safe acos (because of rounding errors in previous calculations which result in an input outside valid bounds)
inline double acos2(double in){
	return acos(Clamp(in, -1.0, 1.0));
}

//! safe asin (because of rounding errors in previous calculations which result in an input outside valid bounds)
inline double asin2(double in){
	return asin(Clamp(in, -1.0, 1.0));
}

//! round A to B
template <class A, class B>
B rd(A in){
	bool sign = in<(A)0.0;
	in = sign?-in:in;//abs
	B out = (B)(in+(A)0.5);
	return sign?-out:out;
}

//! A (in), B (out)
template <typename A, typename B>
irr::core::vector2d<B> rdVector2D(const irr::core::vector2d<A>& v){
	return irr::core::vector2d<B>(rd<A,B>(v.X), rd<A,B>(v.Y));
}

//! A (in), B (out)
template <typename A, typename B>
irr::core::vector2d<B> rdVector3D(const irr::core::vector3d<A>& v){
	return irr::core::vector3d<B>(rd<A,B>(v.X), rd<A,B>(v.Y), rd<A,B>(v.Z));
}

//returns a vector who points to the left side (precondition: Y-Achse to bottom, X Achse to right side)
template <class T> 
irr::core::vector2d<T> leftSide(irr::core::vector2d<T> v){
	return irr::core::vector2d<T>(v.Y, -v.X);
}

//! creates a new rectangle with the given aspect ratio inside the given rectangle while using the maximum available space and being centered
template <typename TRectValue, typename TRatioValue>
irr::core::rect<TRectValue> makeXic(const irr::core::rect<TRectValue>& viewPort, TRatioValue widthPerHeight){
	TRectValue w = viewPort.getWidth(), h = viewPort.getHeight();
	if(widthPerHeight*h<=w){//h constant
		TRectValue newWDiv2 = (widthPerHeight*h)/2;
		TRectValue M = (viewPort.LowerRightCorner.X+viewPort.UpperLeftCorner.X)/2;
		return irr::core::rect<TRectValue>(M-newWDiv2, viewPort.UpperLeftCorner.Y, M+newWDiv2, viewPort.LowerRightCorner.Y);
	}else{//w konstant
		TRectValue newHDiv2 = (w/widthPerHeight)/2;
		TRectValue M = (viewPort.LowerRightCorner.Y+viewPort.UpperLeftCorner.Y)/2;
		return irr::core::rect<TRectValue>(viewPort.UpperLeftCorner.X, M-newHDiv2, viewPort.LowerRightCorner.X, M+newHDiv2);
	}
}

//! converts a rectangle by casting it's elements
template <typename A, typename B>
irr::core::rect<B> convertRectangle(const irr::core::rect<A>& r){
	return irr::core::rect<B>((A)r.UpperLeftCorner.X, (A)r.UpperLeftCorner.Y, (A)r.LowerRightCorner.X, (A)r.LowerRightCorner.Y);
}

//! converts a vector2d by casting it's elements
template <typename A, typename B>
irr::core::vector2d<B> convertVector2D(const irr::core::vector2d<A>& v){
	return irr::core::vector2d<B>((A)v.X, (A)v.Y);
}

//! converts a vector3d by casting it's elements
template <typename A, typename B>
irr::core::vector3d<B> convertVector3D(const irr::core::vector3d<A>& v){
	return irr::core::vector3d<B>((A)v.X, (A)v.Y, (A)v.Z);
}

template <typename TValue>
TValue Angle0to360(TValue phi){
	phi = fmod(phi, (TValue)360.0);
	return phi<(TValue)0.0?(phi+(TValue)360.0):phi;
}

//! returns true if the intervals [a1,a2] and [b1,b2] overlap
template <class T>
inline bool NumbersOverlap(T a1, T a2, T b1, T b2){
	return (b1<=a1 && b2>=a1) || (b1<=a2 && b2>=a2) || (b1>=a1 && b2<=a2);
}

#endif
