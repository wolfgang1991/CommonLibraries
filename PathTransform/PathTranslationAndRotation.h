#ifndef PathTranslationAndRotation_H_INCLUDED
#define PathTranslationAndRotation_H_INCLUDED

#include "IPathTransform.h"
#include <Matrix.h>
#include <Duernion.h>
#include <sstream>
#include <cstdint>
#include <iostream>

//! use TFloat to specify the floating point scalar precision e.g. double
template <typename TPath, typename TFloat>
class PathTranslationAndRotation : public IPathTransform<TPath>{

	protected:
	
	typedef typename IPathTransform<TPath>::Scalar Scalar;
	
	Vector2D<TFloat> pos;//! Translation
	Duernion<TFloat> ori;//! Orientation Duernion
	
	Matrix<3,3,TFloat> homo;//! homogenous transformation matrix calculated from pos and ori
	
	virtual void updateMatrix(){
		auto R = createSubMatrix(homo,0,0,2,2);//rotation submatrix of homo matrix
		auto T = createSubMatrix(homo,0,2,2,1);//translation submatrix of homo matrix
		R = ori.convertToMatrix();
		T = pos;
	}
	
	//! Updates the transformation using a new pair of matched points. Model estimation is similar to Kalman Filter measurements updates but using slerp instead of linear transformations.
	//! usedPairCount is automatically increased and is used for calculating the correct weights
	void updateByPair(const MatchedPoint<Scalar>& a, const MatchedPoint<Scalar>& b, uint32_t& usedPairCount){
		usedPairCount++;
		//Calc new transformation
		Vector2D<TFloat> sourceB{b.sourcePoint[0], b.sourcePoint[1]}, sourceA{a.sourcePoint[0], a.sourcePoint[1]};
		Vector2D<TFloat> measB{b.measuredPoint[0], b.measuredPoint[1]}, measA{a.measuredPoint[0], a.measuredPoint[1]};
		Duernion<TFloat> dsource(sourceB-sourceA);
		Duernion<TFloat> dmeas(measB-measA);
		Duernion<TFloat> dresult = dmeas * dsource.getInverse();
		Vector2D<TFloat> sourceCenter = 0.5*(sourceA+sourceB);
		Vector2D<TFloat> measCenter = 0.5*(measA+measB);
		auto translation = measCenter + dresult.transform(-sourceCenter);
		//Interpolate
		TFloat time = ((TFloat)1)/((TFloat)usedPairCount);
		pos = lerp(pos, translation, time);
		ori.slerp(ori, dresult, time);
	}
	
	size_t parseRepresentation(const std::string& representation){
		size_t offset = 0; parseNextCSV(offset, representation);
		pos.get(0) = parseNextCSVTo<TFloat>(offset, representation);
		pos.get(1) = parseNextCSVTo<TFloat>(offset, representation);
		ori.x = parseNextCSVTo<TFloat>(offset, representation);
		ori.y = parseNextCSVTo<TFloat>(offset, representation);
		return offset;
	}

	public:
	
	//! TContainer container of matched points
	template <typename TContainer>
	PathTranslationAndRotation(const TContainer& matchedPoints):pos(MatrixInit::ZERO),homo(MatrixInit::IDENTITY){
		static_assert(std::is_floating_point<TFloat>::value);
		uint32_t usedPairCount = 0;//translation from first point is not a pair => 0 required for correct weights
		auto it=matchedPoints.begin();
		if(it!=matchedPoints.end()){
			pos.get(0) = it->measuredPoint[0]-it->sourcePoint[0];
			pos.get(1) = it->measuredPoint[1]-it->sourcePoint[1];
			++it;
			for(; it!=matchedPoints.end(); ++it){
				for(auto it2=matchedPoints.begin(); it2!=it; ++it2){
					updateByPair(*it2, *it, usedPairCount);
				}
			}
		}
		updateMatrix();
	}
	
	PathTranslationAndRotation(const std::string& representation){
		parseRepresentation(representation);
		updateMatrix();
	}
	
	PathTranslationAndRotation():pos(MatrixInit::ZERO),homo(MatrixInit::IDENTITY){}
	
	static const std::string id;
	
	virtual const std::string& getIdentifier(){
		return id;
	}
	
	virtual std::string createStringRepresentation(){
		std::stringstream ss; ss << getIdentifier() << "," << pos.get(0) << "," << pos.get(1) << "," << ori.x << "," << ori.y;
		return ss.str();
	}
	
	virtual void transformSingle(typename IPathTransform<TPath>::Vector& vector){
		Vector3D<TFloat> v{vector[0], vector[1], (TFloat)1};
		v = homo * v;
		vector[0] = v.get(0);
		vector[1] = v.get(1);
	}

};

template <typename TPath, typename TFloat>
const std::string PathTranslationAndRotation<TPath,TFloat>::id = "PathTranslationAndRotation";

#endif
