#ifndef PathTransformCreation_H_INCLUDED
#define PathTransformCreation_H_INCLUDED

#include <PathTranslation.h>
#include <PathTranslationAndRotation.h>
#include <PathTranslationRotationAndScale.h>
#include <TriangulatedPathTransfrom.h>

//! falls back to TRANSLATION or TRANSLATION_AND_ROTATION if not enough points
//! TContainer is a container of matched points
//! lineDistance: distance on a path line after which a subdivision can occur
//! minTriangleAreaWithNeigbors: area of a removable triangle (for a removable vertex) for optimization of subdivided paths
//! distanceEPS: two points are equal if they are closer than distanceEPS
template<typename TPath, typename TFloat, typename TContainer>
IPathTransform<TPath>* createPathTransform(typename IPathTransform<TPath>::Type requestedType, const TContainer& matchedPoints, TFloat lineDistance, TFloat minTriangleAreaWithNeigbors, TFloat distanceEPS = (TFloat)0.0001){
	if(requestedType==IPathTransform<TPath>::TRANSLATION){
		return new PathTranslation<TPath>(matchedPoints);
	}else if(requestedType==IPathTransform<TPath>::TRANSLATION_AND_ROTATION){
		return new PathTranslationAndRotation<TPath, TFloat>(matchedPoints);
	}else if(requestedType==IPathTransform<TPath>::TRANSLATION_ROTATION_AND_SCALE){
		return new PathTranslationRotationAndScale<TPath, TFloat>(matchedPoints);
	}else if(requestedType==IPathTransform<TPath>::FULL_TRIANGULATED){
		#ifdef NO_OPENCV
		return new PathTranslationRotationAndScale<TPath, TFloat>(matchedPoints);
		#else
		if(matchedPoints.size()<3){
			return new PathTranslationRotationAndScale<TPath, TFloat>(matchedPoints);
		}else{
			return new TriangulatedPathTransfrom<TPath, TFloat>(matchedPoints, lineDistance, minTriangleAreaWithNeigbors, distanceEPS);
		}
		#endif
	}else{
		return NULL;
	}
}

template<typename TPath, typename TFloat>
IPathTransform<TPath>* createPathTransform(const std::string& representation){
	size_t offset = 0;
	std::string id = parseNextCSV(offset, representation);
	if(id.compare(TriangulatedPathTransfrom<TPath, TFloat>::id)==0){
		return new TriangulatedPathTransfrom<TPath, TFloat>(representation);
	}else if(id.compare(PathTranslation<TPath>::id)==0){
		return new PathTranslation<TPath>(representation);
	}else if(id.compare(PathTranslationAndRotation<TPath, TFloat>::id)==0){
		return new PathTranslationAndRotation<TPath, TFloat>(representation);
	}else if(id.compare(PathTranslationRotationAndScale<TPath, TFloat>::id)==0){
		return new PathTranslationRotationAndScale<TPath, TFloat>(representation);
	}
	return NULL;
}

#endif
