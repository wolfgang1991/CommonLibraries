#ifndef IPathTransform_H_INCLUDED
#define IPathTransform_H_INCLUDED

#include <Matrix.h>
#include <StringHelpers.h>

#include <string>
#include <array>

//! Interface for Path Transformations, TPath is a container of "vectors". A "vector" is a one column matrix with random access of scalars (e.g. integers or floats)
template <typename TPath>
class IPathTransform{
	
	public:
	
	enum Type{
		TRANSLATION,
		TRANSLATION_AND_ROTATION,
		TRANSLATION_ROTATION_AND_SCALE,
		FULL_TRIANGULATED,
		TYPE_COUNT
	};
	
	typedef typename TPath::value_type Vector;
	typedef typename Vector::value_type Scalar;
	
	virtual ~IPathTransform(){}
	
	virtual const std::string& getIdentifier() = 0;
	
	//! The prefix of a string representation is always the identifier
	virtual std::string createStringRepresentation() = 0;
	
	//! Transforms a single point/vector (only the first two components are transformed)
	virtual void transformSingle(Vector& vector) = 0;
	
	//! In Place transformation without the possibility for subdivisions
	virtual void transformInPlace(TPath& path){
		for(auto it = path.begin(); it!=path.end(); ++it){
			transformSingle(*it);
		}
	}
	
	//! Transforms the path and returns a new path (asserts return value optimization for efficiency) (only the first two components are transformed)
	//! A transformation wich also implements/requires subdivisions needs to override this method.
	virtual TPath transform(const TPath& path){
		TPath newPath(path);
		transformInPlace(newPath);
		return newPath;
	}
	
};

//! A pair of source and matched points to define a transformation. The resulting transformation shall always transform sourcePoints to measuredPoints as close as possible
template <typename T>
class MatchedPoint{
	
	public:
	
	typedef T value_type;
	
	Vector2D<T> sourcePoint;
	Vector2D<T> measuredPoint;
	
	//! useful for sorting
	bool operator<(const MatchedPoint& other) const{
		return sourcePoint.get(0)<other.sourcePoint.get(0);
	}
	
};

inline std::string parseNextCSV(size_t& offset, const std::string& csvLine){
	if(offset<csvLine.size()){
		 size_t next = csvLine.find(',', offset);
		 std::string res = csvLine.substr(offset, next-offset);
		 offset = next!=std::string::npos?(next+1):next;
		 return res;
	}
	return "";
}

//! useful for constructing from string representations; changes the offset
template <typename TValue>
TValue parseNextCSVTo(size_t& offset, const std::string& csvLine){
	return convertStringTo<TValue>(parseNextCSV(offset, csvLine));
}

#endif
