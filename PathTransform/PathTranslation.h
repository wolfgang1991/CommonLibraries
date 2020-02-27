#ifndef PathTranslation_H_INCLUDED
#define PathTranslation_H_INCLUDED

#include "IPathTransform.h"
#include "Matrix.h"
#include <sstream>
#include <cstdint>
#include <iostream>

template <typename TPath>
class PathTranslation : public IPathTransform<TPath>{

	private:
	
	typedef typename IPathTransform<TPath>::Scalar Scalar;
	
	Vector2D<Scalar> pos;

	public:
	
	//! TContainer container of matched points
	template <typename TContainer>
	PathTranslation(const TContainer& matchedPoints):pos(MatrixInit::ZERO){
		Scalar cnt = 0;
		for(auto it=matchedPoints.begin(); it!=matchedPoints.end(); ++it){
			const MatchedPoint<Scalar>& m = *it;
			pos.get(0) += m.measuredPoint[0]-m.sourcePoint[0];
			pos.get(1) += m.measuredPoint[1]-m.sourcePoint[1];
			cnt++;
		}
		if(cnt>0){
			pos.get(0) /= cnt;
			pos.get(1) /= cnt;
		}
	}
	
	PathTranslation(const std::string& representation){
		size_t offset = 0; parseNextCSV(offset, representation);
		pos.get(0) = parseNextCSVTo<Scalar>(offset, representation);
		pos.get(1) = parseNextCSVTo<Scalar>(offset, representation);
	}
	
	static const std::string id;
	
	const std::string& getIdentifier(){
		return id;
	}
	
	std::string createStringRepresentation(){
		std::stringstream ss; ss << id << "," << pos.get(0) << "," << pos.get(1);
		return ss.str();
	}
	
	void transformSingle(typename IPathTransform<TPath>::Vector& vector){
		vector[0] += pos.get(0);
		vector[1] += pos.get(1);
	}

};

template <typename TPath>
const std::string PathTranslation<TPath>::id = "PathTranslation";

#endif
