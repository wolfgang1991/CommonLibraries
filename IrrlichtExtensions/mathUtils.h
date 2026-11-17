#ifndef MATHUTILS_H_INCLUDED
#define MATHUTILS_H_INCLUDED

#include <rect.h>
#include <vector2d.h>
#include <vector3d.h>
#include <mathHelpers.h>

#include <cmath>

#define DOUBLE_EPS 0.00000001
#define FLOAT_EPS 0.0001

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

#endif
