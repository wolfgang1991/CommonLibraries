#ifndef TriangulatedPathTransfrom_H_INCLUDED
#define TriangulatedPathTransfrom_H_INCLUDED

#include "SubdividedPathTransform.h"

#ifndef NO_OPENCV
#include <opencv2/imgproc.hpp>
#endif

#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

#define MIN_TRIANGLE_AREA 0.00001

//! use TFloat to specify the floating point scalar precision e.g. double
//! if it is constructed without a given triangulation the triangulation is calculated via opencv
template <typename TPath, typename TFloat>
class TriangulatedPathTransfrom : public SubdividedPathTransform<TPath, TFloat>{

	protected:
	
	typedef SubdividedPathTransform<TPath, TFloat> Super;
	typedef typename Super::Scalar Scalar;
	
	struct TriangleTransformation{
		TFloat triangleArea;//used to determine whether toBarycentric is usable (numeric issues) and to determine distances from edges using barycentric coordinated
		Matrix<3,3,TFloat> toBarycentric;//homogenous cartesian coordinates in source space to barycentric coordinates; MUST NOT BE USED IF triangleArea is close to zero
		Matrix<2,3,TFloat> toTransformedCartesian;//barycentric coordinates to (non homogenous) cartesion coordinates in measured space
		Vector2D<TFloat> m1m0, m2m0, m2m1;
		TFloat m1m0Len, m2m0Len, m2m1Len;
		Vector2D<TFloat> m0, m1, m2;
	};
	
	std::vector<MatchedPoint<Scalar>> points;
	std::vector<uint32_t> indices;//! a triangle is formed every 3 indices
	std::vector<TriangleTransformation> transformations;

	TFloat calcDistanceFromLine(const Vector2D<TFloat>& point, const Vector2D<TFloat>& start, const Vector2D<TFloat>& end, TFloat endStartLength, const Vector2D<TFloat>& endStartNormalized){
		Vector2D<TFloat> dps = point-start;
		TFloat projLength = calcDotProduct(dps,endStartNormalized);
		if(projLength<0){
			return calcFrobeniusNorm(dps);
		}else if(projLength>endStartLength){
			return calcFrobeniusNorm(point-end);
		}else{
			return calcFrobeniusNorm(dps-projLength*endStartNormalized);
		}
	}
	
	Vector3D<TFloat> calcDistancesFromEdges(const Vector2D<TFloat>& point, const TriangleTransformation& t){
		return Vector3D<TFloat>{calcDistanceFromLine(point, t.m1, t.m2, t.m2m1Len, t.m2m1), calcDistanceFromLine(point, t.m0, t.m2, t.m2m0Len, t.m2m0), calcDistanceFromLine(point, t.m0, t.m1, t.m1m0Len, t.m1m0)};
	}
	
	//! finds a point inside the sorted "points" vector average case O(logn), worst case O(n) but in the worst case there's also no meaningful triangulation
	uint32_t findSourcePoint(const Vector2D<Scalar>& point, TFloat distanceEPS) const{
		auto it = std::lower_bound(points.begin(), points.end(), MatchedPoint<Scalar>{point,{(Scalar)0,(Scalar)0}});//O(logn)
		int32_t p = it-points.begin();
		int32_t m = p-1;
		bool pOk = true, mOk = true;
		while(pOk || mOk){//small linear search
			pOk = p<(int32_t)points.size();
			if(pOk){
				if(calcFrobeniusNorm<Vector2D<Scalar>, TFloat>(points[p].sourcePoint-point)<distanceEPS){
					return p;
				}
				p++;
			}
			mOk = m>=0;
			if(mOk){
				if(calcFrobeniusNorm<Vector2D<Scalar>, TFloat>(points[m].sourcePoint-point)<distanceEPS){
					return m;
				}
				m--;
			}
		}
		return 0;//impossible to end up here theoretically if distanceEPS is suitable
	}
	
	public:
	
	static_assert(std::is_floating_point<TFloat>::value);
	
	static bool isInsideTriangle(const Vector3D<TFloat>& barycentricCoords){
		return barycentricCoords.get(0)>=0 && barycentricCoords.get(1)>=0 && barycentricCoords.get(2)>=0;
	}
	
	#ifndef NO_OPENCV
	//! TContainer container of matched points
	template <typename TContainer>
	TriangulatedPathTransfrom(const TContainer& matchedPoints, TFloat lineDistance, TFloat minTriangleAreaWithNeigbors, TFloat distanceEPS = (TFloat)0.0001):Super(lineDistance, minTriangleAreaWithNeigbors){
		assert(matchedPoints.size()>=3);
		points.reserve(matchedPoints.size());
		std::copy(std::begin(matchedPoints), std::end(matchedPoints), std::back_inserter(points));
		std::sort(points.begin(), points.end());
		Scalar minX, minY, maxX, maxY;
		minX = minY = std::numeric_limits<Scalar>::max();
		maxX = maxY = std::numeric_limits<Scalar>::min();
		for(uint32_t i=0; i<points.size(); i++){
			Vector2D<Scalar>& sourcePoint = points[i].sourcePoint;
			if(sourcePoint[0]<minX){minX = sourcePoint[0];}
			if(sourcePoint[0]>maxX){maxX = sourcePoint[0];}
			if(sourcePoint[1]<minY){minY = sourcePoint[1];}
			if(sourcePoint[1]>maxY){maxY = sourcePoint[1];}
		}
		cv::Subdiv2D sd(cv::Rect(minX-10,minY-10,maxX-minX+20,maxY-minY+20));
		for(uint32_t i=0; i<points.size(); i++){
			Vector2D<Scalar>& sourcePoint = points[i].sourcePoint;
			sd.insert(cv::Point2f(sourcePoint[0], sourcePoint[1]));
		}
		std::vector<cv::Vec6f> triangleList;
		sd.getTriangleList(triangleList);
		indices.resize(triangleList.size()*3);
		for(uint32_t i=0; i<triangleList.size(); i++){
			cv::Vec6f& v = triangleList[i];
			for(uint32_t j=0; j<3; j++){
				indices[i*3+j] = findSourcePoint(Vector2D<Scalar>(rdScalar<cv::Vec6f::value_type,Scalar>(v[2*j]), rdScalar<cv::Vec6f::value_type,Scalar>(v[2*j+1])), distanceEPS);
			}
		}
		recalculateTransformations();
	}
	#endif
	
	TriangulatedPathTransfrom(const std::string& representation):Super(1,1){
		size_t offset = Super::parseRepresentation(representation);
		uint32_t pointCount = parseNextCSVTo<uint32_t>(offset, representation);
		points.resize(pointCount);
		for(uint32_t i=0; i<pointCount; i++){
			MatchedPoint<Scalar>& m = points[i];
			m.sourcePoint[0] = parseNextCSVTo<Scalar>(offset, representation);
			m.sourcePoint[1] = parseNextCSVTo<Scalar>(offset, representation);
			m.measuredPoint[0] = parseNextCSVTo<Scalar>(offset, representation);
			m.measuredPoint[1] = parseNextCSVTo<Scalar>(offset, representation);
		}
		uint32_t indexCount = parseNextCSVTo<uint32_t>(offset, representation);
		indices.resize(indexCount);
		for(uint32_t i=0; i<indexCount; i++){
			indices[i] = parseNextCSVTo<uint32_t>(offset, representation);
		}
		recalculateTransformations();
	}
	
	void recalculateTransformations(){
		transformations.clear();
		uint32_t tcnt = indices.size()/3;
		transformations.resize(tcnt);
		for(uint32_t i=0; i<tcnt; i++){
			MatchedPoint<Scalar>& m0 = points[indices[3*i]];
			MatchedPoint<Scalar>& m1 = points[indices[3*i+1]];
			MatchedPoint<Scalar>& m2 = points[indices[3*i+2]];
			TriangleTransformation& t = transformations[i];
			t.m0 = convertMatrix<TFloat>(m0.sourcePoint);
			t.m1 = convertMatrix<TFloat>(m1.sourcePoint);
			t.m2 = convertMatrix<TFloat>(m2.sourcePoint);
			t.m1m0 = t.m1-t.m0;
			t.m2m0 = t.m2-t.m0;
			t.m2m1 = t.m2-t.m1;
			t.triangleArea = ((TFloat)0.5)*fabs(calcDeterminant(Matrix<2,2,TFloat>{
				t.m1m0.get(0),	t.m2m0.get(0),
				t.m1m0.get(1),	t.m2m0.get(1)}));
			t.toBarycentric = calcInverse(Matrix<3,3,TFloat>{
				t.m0.get(0), t.m1.get(0), t.m2.get(0),
				t.m0.get(1), t.m1.get(1), t.m2.get(1),
				1,           1,           1});
			t.toTransformedCartesian = Matrix<2,3,TFloat>{
				m0.measuredPoint.get(0), m1.measuredPoint.get(0), m2.measuredPoint.get(0),
				m0.measuredPoint.get(1), m1.measuredPoint.get(1), m2.measuredPoint.get(1)};
			t.m1m0Len = calcFrobeniusNorm<decltype(t.m1m0), TFloat>(t.m1m0);
			t.m1m0 = t.m1m0 * ((TFloat)1)/t.m1m0Len;
			t.m2m0Len = calcFrobeniusNorm<decltype(t.m2m0), TFloat>(t.m2m0);
			t.m2m0 = t.m2m0 * ((TFloat)1)/t.m2m0Len;
			t.m2m1Len = calcFrobeniusNorm<decltype(t.m2m1), TFloat>(t.m2m1);
			t.m2m1 = t.m2m1 * ((TFloat)1)/t.m2m1Len;
		}
	}
	
	static const std::string id;
	
	const std::string& getIdentifier(){return id;}
	
	std::string createStringRepresentation(){
		std::stringstream ss; ss << Super::createStringRepresentation() << "," << points.size();
		for(uint32_t i=0; i<points.size(); i++){
			MatchedPoint<Scalar>& m = points[i];
			ss << "," << m.sourcePoint[0] << "," << m.sourcePoint[1] << "," << m.measuredPoint[0] << "," << m.measuredPoint[1];
		}
		ss << "," << indices.size();
		for(uint32_t i=0; i<indices.size(); i++){
			ss << "," << indices[i];
		}
		return ss.str();
	}
	
	void fillResult(typename IPathTransform<TPath>::Vector& vector, const TriangleTransformation& t, const Vector3D<TFloat>& barycentric){
		SubMatrix<0, 0, 2, 1, typename IPathTransform<TPath>::Vector> fill2d(vector);
		fill2d = rdMatrix<typename IPathTransform<TPath>::Scalar>(t.toTransformedCartesian * barycentric);
	}
	
	void transformSingle(typename IPathTransform<TPath>::Vector& vector){
		if(!transformations.empty()){
			Vector3D<TFloat> v{vector[0], vector[1], (TFloat)1};
			Vector2D<TFloat> v2d{vector[0], vector[1]};
			TFloat smallestDist = std::numeric_limits<TFloat>::max();
			Vector3D<TFloat> closestBary(MatrixInit::IDENTITY);
			uint32_t closestIndex = 0;
			for(uint32_t i=0; i<transformations.size(); i++){
				TriangleTransformation& t = transformations[i];
				if(t.triangleArea>MIN_TRIANGLE_AREA){
					Vector3D<TFloat> barycentric = t.toBarycentric * v;
					if(isInsideTriangle(barycentric)){
						fillResult(vector, t, barycentric);//vector = rdMatrix<typename IPathTransform<TPath>::Scalar>(t.toTransformedCartesian * barycentric);
						return;
					}else{
						TFloat minDist = getMatrixMin(calcDistancesFromEdges(v2d, t));
						if(minDist<smallestDist){
							closestBary = barycentric;
							closestIndex = i;
							smallestDist = minDist;
						}
					}
				}
			}
			//Fallback: Use the transformation of the triangle with the smallest distance to one of the edges
			fillResult(vector, transformations[closestIndex], closestBary);//vector = rdMatrix<typename IPathTransform<TPath>::Scalar>(transformations[closestIndex].toTransformedCartesian * closestBary);
		}
	}

};

template <typename TPath, typename TFloat>
const std::string TriangulatedPathTransfrom<TPath,TFloat>::id = "TriangulatedPathTransfrom";

#endif
