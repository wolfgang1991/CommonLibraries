#ifndef PathTranslationRotationAndScale_H_INCLUDED
#define PathTranslationRotationAndScale_H_INCLUDED

#include "PathTranslationAndRotation.h"

#include <vector>

template <typename TPath, typename TFloat>
class PathTranslationRotationAndScale : public PathTranslationAndRotation<TPath,TFloat>{

	private:
	
	TFloat floatAbs(TFloat v) const{
		return v<(TFloat)0.0?-v:v;
	}
	
	protected:
	
	typedef typename IPathTransform<TPath>::Scalar Scalar;
	typedef PathTranslationAndRotation<TPath,TFloat> Super;
	
	Vector2D<TFloat> scale;//! also used to calculate the homo matrix
	Vector2D<TFloat> centerOfMass;//! also used to calculate the homo matrix, describes the inverse translation to distribute all source points around the COM
	
	virtual void updateMatrix(){
		Super::updateMatrix();
		Matrix<3,3,TFloat> scaleMatrix{ // = (inverse COM Translation) * (scale) * (COM Translation) (COM = Center Of Mass)
			scale.get(0),	0,					centerOfMass.get(0)-scale.get(0)*centerOfMass.get(0),
			0,					scale.get(1),	centerOfMass.get(1)-scale.get(1)*centerOfMass.get(1),
			0,					0,					1};
		Super::homo = Super::homo * scaleMatrix;
	}
	
	public:
	
	//! TContainer container of matched points
	template <typename TContainer>
	PathTranslationRotationAndScale(const TContainer& matchedPoints, TFloat scaleEps = (TFloat)0.1):Super(matchedPoints),scale{1,1}{
		//Calculate the inverse "rotation and translation matrix"
		Matrix<3,3,TFloat> invHomo(MatrixInit::IDENTITY);
		Duernion<TFloat> invOri = Super::ori.getInverse();
		auto R = createSubMatrix(invHomo,0,0,2,2);//rotation submatrix of invHomo matrix
		R = invOri.convertToMatrix();
		auto T = createSubMatrix(invHomo,0,2,2,1);//translation submatrix of invHomo matrix
		T = -invOri.transform(Super::pos);
		//Transform measured points into original "path space" (using the inverse)
		std::vector<Vector2D<TFloat>> sourcePoints, transformedMeasPoints;
		uint32_t size = matchedPoints.size();
		sourcePoints.reserve(size);
		transformedMeasPoints.reserve(size);
		for(auto it=matchedPoints.begin(); it!=matchedPoints.end(); ++it){
			const MatchedPoint<Scalar>& m = *it;
			sourcePoints.push_back(Vector2D<TFloat>{m.sourcePoint[0], m.sourcePoint[1]});
			Vector3D<TFloat> tp = invHomo * Vector3D<TFloat>{m.measuredPoint[0], m.measuredPoint[1], 1};
			transformedMeasPoints.push_back(Vector2D<TFloat>{tp.get(0), tp.get(1)});
		}
		//Calculate the center of mass (used to get better / more meaningful scaling results)
		centerOfMass = Vector2D<TFloat>{0,0};
		for(uint32_t i=0; i<size; i++){
			centerOfMass = lerp(centerOfMass, sourcePoints[i], ((TFloat)1)/((TFloat)(i+1)));
		}
		//Calculate the average scale
		scale = Vector2D<TFloat>{1,1};
		for(uint32_t i=0; i<size; i++){
			Vector2D<TFloat> s = sourcePoints[i] - centerOfMass;
			Vector2D<TFloat> m = transformedMeasPoints[i] - centerOfMass;
			scale = lerp(scale, Vector2D<TFloat>{floatAbs(s.get(0))>scaleEps?(m.get(0)/s.get(0)):scale.get(0), floatAbs(s.get(1))>scaleEps?(m.get(1)/s.get(1)):scale.get(1)}, ((TFloat)1)/((TFloat)(i+1)));
		}
		updateMatrix();
	}
	
	PathTranslationRotationAndScale(const std::string& representation){
		size_t offset = Super::parseRepresentation(representation);
		scale.get(0) = parseNextCSVTo<TFloat>(offset, representation);
		scale.get(1) = parseNextCSVTo<TFloat>(offset, representation);
		centerOfMass.get(0) = parseNextCSVTo<TFloat>(offset, representation);
		centerOfMass.get(1) = parseNextCSVTo<TFloat>(offset, representation);
		updateMatrix();
	}
	
	static const std::string id;
	
	virtual const std::string& getIdentifier(){
		return id;
	}
	
	virtual std::string createStringRepresentation(){
		std::stringstream ss; ss << Super::createStringRepresentation() << "," << scale.get(0) << "," << scale.get(1) << "," << centerOfMass.get(0) << "," << centerOfMass.get(1);
		return ss.str();
	}
	
};

template <typename TPath, typename TFloat>
const std::string PathTranslationRotationAndScale<TPath,TFloat>::id = "PathTranslationRotationAndScale";

#endif
