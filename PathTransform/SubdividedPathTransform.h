#ifndef SubdividedPathTransform_H_INCLUDED
#define SubdividedPathTransform_H_INCLUDED

#include "IPathTransform.h"

#include <list>

template <typename TPath, typename TFloat>
class SubdividedPathTransform : public IPathTransform<TPath>{
	
	protected:
	
	TFloat minTriangleAreaWithNeigbors;//Threshold to determine wether the new transformed point shall be really created
	TFloat lineDistance;//Distance after which an additional point shall be created along a line, can be determined by the half minimum distance of sourcepoints
	
	size_t parseRepresentation(const std::string& representation){
		size_t offset = 0; parseNextCSV(offset, representation);
		lineDistance = parseNextCSVTo<TFloat>(offset, representation);
		minTriangleAreaWithNeigbors = parseNextCSVTo<TFloat>(offset, representation);
		return offset;
	}
	
	public:
	
	typedef IPathTransform<TPath> Super;
	typedef typename Super::Vector Vector;
	typedef typename Super::Scalar Scalar;
	
	static_assert(std::is_floating_point<TFloat>::value);
	
	SubdividedPathTransform(TFloat lineDistance, TFloat minTriangleAreaWithNeigbors):minTriangleAreaWithNeigbors(minTriangleAreaWithNeigbors),lineDistance(lineDistance){}
	
	//! subdivides a line if applicable, assumes measStart has already been added and adds transformed additional points if required and adds measEnd
	void subdivide(std::list<Vector>& l, const Vector& sourceStart, const Vector& sourceEnd, const Vector& measStart, const Vector& measEnd){
		Vector d = sourceEnd-sourceStart;
		TFloat len = calcFrobeniusNorm<Vector,TFloat>(d);
		if(len>lineDistance){
			auto newPoint = convertMatrix<TFloat>(sourceStart)+((TFloat)0.5)*convertMatrix<TFloat>(d);
			auto newPointRd = rdMatrix<Scalar>(newPoint);
			auto newPointTransformedRd = newPointRd;
			transformSingle(newPointTransformedRd);
			subdivide(l, sourceStart, newPointRd, measStart, newPointTransformedRd);
			auto it = l.end();
			--it;//points to newPointTransformed pushed back by recursive subdivide call
			--it;//points to first vertex in triangle
			Vector& first = *it;
			subdivide(l, newPointRd, sourceEnd, newPointTransformedRd, measEnd);
			++it;
			++it;//points to last vertex in triangle
			Vector& third = *it;
			Vector m1m0 = third-first;
			Vector m2m0 = newPointTransformedRd-first;
			TFloat triangleArea = ((TFloat)0.5)*fabs(calcDeterminant(Matrix<2,2,TFloat>{
				(TFloat)m1m0.get(0),	(TFloat)m2m0.get(0),
				(TFloat)m1m0.get(1),	(TFloat)m2m0.get(1)}));
			if(triangleArea<minTriangleAreaWithNeigbors){
				--it;
				l.erase(it);
			}//else{std::cout << "triangleArea: " << triangleArea << " minTriangleAreaWithNeigbors: " << minTriangleAreaWithNeigbors << std::endl;}
		}else{
			l.push_back(measEnd);
		}
	}
	
	virtual TPath transform(const TPath& path){
		std::list<Vector> l;
		auto it = path.begin();
		if(it!=path.end()){
			const Vector* last = &(*it);
			Vector start = *it;
			transformSingle(start);
			l.push_back(start);
			++it;
			for(; it!=path.end(); ++it){
				Vector current = *it;
				transformSingle(current);
				subdivide(l, *last, *it, l.back(), current);
				last = &(*it);
			}
		}
		//std::cout << "added vertices: " << (l.size()-path.size()) << std::endl;
		return TPath(l.begin(), l.end());
	}
	
	virtual void transformSingle(Vector& vector) = 0;
	
	virtual std::string createStringRepresentation(){
		std::stringstream ss; ss << getIdentifier() << "," << lineDistance << "," << minTriangleAreaWithNeigbors;
		return ss.str();
	}
	
	virtual const std::string& getIdentifier() = 0;
	
};

#endif
