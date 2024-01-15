#ifndef uCMatrix_H_
#define uCMatrix_H_

#include "platforms.h"

#include <stdint.h>

#ifndef MICROCONTROLLER_PLATFORM
#include <ostream>
#include <iomanip>
#endif

namespace ucstd{

//! Yet another very simple matrix implementation without STL, suitable for microcontrollers
template<typename TScalar, uint32_t rowCount, uint32_t columnCount>
class Matrix{
	
	public:
	
	enum MatrixInit{
		UNDEFINED,
		ZERO,
		IDENTITY,
		INIT_COUNT
	};
	
	static constexpr uint32_t size = rowCount * columnCount;
	TScalar data[size];
	
	template <typename... T> 
	Matrix(T... ts):data{static_cast<TScalar>(ts)...} {}
	
	Matrix(MatrixInit init){
		if(init==ZERO){
			setZero();
		}else if(init==IDENTITY){
			setIdentity();
		}
	}
	
	void setZero(){
		for(uint32_t i=0; i<size; i++){
			data[i] = 0;
		}
	}
	
	void setIdentity(){
		for(uint32_t x=0; x<columnCount; x++){
			for(uint32_t y=0; y<rowCount; y++){
				if(x==y){
					set(y, x, 1);
				}else{
					set(y, x, 0);
				}
			}
		}
	}
	
	void set(uint32_t rowIndex, uint32_t columnIndex, TScalar s){
		data[columnIndex + columnCount*rowIndex] = s;
	}
	
	TScalar get(uint32_t rowIndex, uint32_t columnIndex) const{
		return data[columnIndex + columnCount*rowIndex];
	}
	
	TScalar& get(uint32_t rowIndex, uint32_t columnIndex){
		return data[columnIndex + columnCount*rowIndex];
	}
	
	TScalar& operator[](uint32_t index){
		return data[index];
	}
	
	const TScalar& operator[](uint32_t index) const{
		return data[index];
	}
	
	TScalar calcSquareSumNorm() const{
		TScalar sum = 0;
		for(uint32_t i=0; i<size; i++){
			sum += (data[i]*data[i]);
		}
		return sum;
	}
	
	//! assumes TScalar is a floating point type
	TScalar calcFrobeniusNorm() const{
		return sqrt(calcSquareSumNorm());
	}

	template<uint32_t otherColumnCount>
	Matrix<TScalar, rowCount, otherColumnCount> operator*(const Matrix<TScalar, columnCount, otherColumnCount>& other) const{
		Matrix<TScalar, rowCount, otherColumnCount> res(UNDEFINED);
		for(uint32_t oX=0; oX<otherColumnCount; oX++){
			for(uint32_t y=0; y<rowCount; y++){
				TScalar acc = 0;//dot product of yth row with oXth column
				for(uint32_t x=0; x<columnCount; x++){
					acc += get(y,x) * other.get(x,oX);
				}
				res.set(y, oX, acc);
			}
		}
		return res;
	}
	
	bool operator==(const Matrix<TScalar, rowCount, columnCount>& other) const{
		bool isEqual = true;
		for(uint32_t i=0; i<size && isEqual; i++){
			isEqual = data[i]==other.data[i];
		}
		return isEqual;
	}
	
};

template<typename TScalar, uint32_t size>
using Vector = Matrix<TScalar, size, 1>;

template<typename TScalar>
using Vector3D = Vector<TScalar, 3>;

using Vector3DF = Vector3D<float>;

template<typename TScalar>
using Matrix3D = Matrix<TScalar, 3, 3>;

using Matrix3DF = Matrix3D<float>;

template<typename TScalar, uint32_t size>
TScalar calcDotProduct(const Vector<TScalar, size>& a, const Vector<TScalar, size>& b){
	TScalar sum = 0;
	for(uint32_t i=0; i<size; i++){
		sum += a.data[i] * b.data[i];
	}
	return sum;
}

//! in radians, TScalar needs to be floating point, length of vectors must be != 0
template<typename TScalar, uint32_t size>
TScalar calcAngle(const Vector<TScalar, size>& a, const Vector<TScalar, size>& b){
	return acos(calcDotProduct(a,b) / (a.calcFrobeniusNorm()*b.calcFrobeniusNorm()) );
}

//! Rotation around first axis
template <typename TScalar>
Matrix3D<TScalar> createPitchMatrix3D(TScalar phi){
	TScalar cosPhi = cos(phi);
	TScalar sinPhi = sin(phi);
	return Matrix3D<TScalar>{1, 0, 0,
		      0, cosPhi, -sinPhi,
		      0, sinPhi, cosPhi};
}

//! Rotation around second axis
template <typename TScalar>
Matrix3D<TScalar> createYawMatrix3D(TScalar phi){
	TScalar cosPhi = cos(phi);
	TScalar sinPhi = sin(phi);
	return Matrix3D<TScalar>{cosPhi, 0, sinPhi,
		      0, 1, 0,
		      -sinPhi, 0, cosPhi};
}

//! Rotation around third axis
template <typename TScalar>
Matrix3D<TScalar> createRollMatrix3D(TScalar phi){
	TScalar cosPhi = cos(phi);
	TScalar sinPhi = sin(phi);
	return Matrix3D<TScalar>{cosPhi, -sinPhi, 0,
		      sinPhi, cosPhi, 0,
		      0, 0, 1};
}

constexpr float pi = 3.141592653589793f;
constexpr float degRad = 180.f/pi;//! °/rad

#ifndef MICROCONTROLLER_PLATFORM
//! Output Matrices
template<typename TScalar, uint32_t rowCount, uint32_t columnCount>
std::ostream& operator<<(std::ostream &out, const Matrix<TScalar, rowCount, columnCount>& m){
	for(uint32_t y=0; y<rowCount; y++){
		for(uint32_t x=0; x<columnCount; x++){
			out << std::setw(15) << std::right << m.get(y,x);
		}
		out << "\n";
	}
	return out; 
}
#endif

}

#endif
