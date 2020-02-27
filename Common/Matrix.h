#ifndef Matrix_H_INCLUDED
#define Matrix_H_INCLUDED

#include <cstdint>
#include <functional>
#include <array>
#include <ostream>
#include <iomanip>
#include <cmath>

//! Serveral Matrix implementations are defined here. Operations are generically defined outside of matrix classes and basic stuff like get/set and init inside the matrices to exploit compile time polymorphism

enum class MatrixInit{
	UNDEFINED,
	ZERO,
	IDENTITY,
	INIT_COUNT
};

//! A flexible matrix representation
template <uint32_t TRowCount, uint32_t TColumnCount, typename TScalar>
class Matrix{
	
	protected:
	
	std::array<TScalar, TRowCount*TColumnCount> data;
	
	public:
	
	typedef TScalar Scalar;
	static constexpr uint32_t rowCount = TRowCount;
	static constexpr uint32_t columnCount = TColumnCount;
	static constexpr bool isMatrix = true;
	
	const Scalar& get(uint32_t row, uint32_t column = 0) const{
		return data[column+columnCount*row];
	}
	
	Scalar& get(uint32_t row, uint32_t column = 0){
		return data[column+columnCount*row];
	}
	
	const Scalar& operator[](uint32_t row) const{
		return get(row, 0);
	}
	
	Scalar& operator[](uint32_t row){
		return get(row, 0);
	}
	
	void set(uint32_t row, uint32_t column, TScalar value){
		 get(row, column) = value;
	}
	
	Matrix(MatrixInit init = MatrixInit::UNDEFINED){
		if(init==MatrixInit::ZERO){
			visitMatrix(*this, [](uint32_t row, uint32_t column, TScalar& value){value = (TScalar)0;});
		}else if(init==MatrixInit::IDENTITY){
			visitMatrix(*this, [](uint32_t row, uint32_t column, TScalar& value){value = row==column?(TScalar)1:(TScalar)0;});
		}
	}
	
	//Init internal array directly
	template <typename... T> 
	Matrix(T... ts):data{ts...} {}
   
	template<typename TMatrix>
	Matrix& operator=(const TMatrix& matrix){
		return copyMatrix(matrix, *this);
	}
	
};

//! A representation of a transpose matrix without the need to copy something
template <typename TParentMatrix>
class TransposeMatrix{

	public:
	
	typedef typename TParentMatrix::Scalar Scalar;
	static constexpr uint32_t rowCount = TParentMatrix::columnCount;
	static constexpr uint32_t columnCount = TParentMatrix::rowCount;
	static constexpr bool isMatrix = TParentMatrix::isMatrix;
	
	TParentMatrix& parentMatrix;
	
	TransposeMatrix(TParentMatrix& parentMatrix):parentMatrix(parentMatrix){
		static_assert(isMatrix);
	}
	
	const Scalar& get(uint32_t row, uint32_t column = 0) const{
		return parentMatrix.get(column, row);
	}
	
	Scalar& get(uint32_t row, uint32_t column = 0){
		return parentMatrix.get(column, row);
	}
	
	void set(uint32_t row, uint32_t column, Scalar value){
		 parentMatrix.set(column, row, value);
	}
	
	template<typename TMatrix>
	TransposeMatrix& operator=(const TMatrix& matrix){
		return copyMatrix(matrix, *this);
	}

};

#define createTransposeMatrix(M) TransposeMatrix<decltype(M)>(M)

//! A representation of a part of a parent matrix by referencing it which is especially useful to fill parts (such as vectors) of a matrix.
template <uint32_t TRowIndex, uint32_t TColumnIndex, uint32_t TRowCount, uint32_t TColumnCount, typename TParentMatrix>
class SubMatrix{

	public:
	
	typedef typename TParentMatrix::Scalar Scalar;
	static constexpr uint32_t rowCount = TRowCount;
	static constexpr uint32_t columnCount = TColumnCount;
	static constexpr bool isMatrix = TParentMatrix::isMatrix;
	
	TParentMatrix& parentMatrix;
	
	SubMatrix(TParentMatrix& parentMatrix):parentMatrix(parentMatrix){
		static_assert(isMatrix);
		static_assert(TRowIndex+TRowCount<=TParentMatrix::rowCount && TColumnIndex+TColumnCount<=TParentMatrix::columnCount);
	}
	
	const Scalar& get(uint32_t row, uint32_t column = 0) const{
		return parentMatrix.get(TRowIndex+row, TColumnIndex+column);
	}
	
	Scalar& get(uint32_t row, uint32_t column = 0){
		return parentMatrix.get(TRowIndex+row, TColumnIndex+column);
	}
	
	void set(uint32_t row, uint32_t column, Scalar value){
		 parentMatrix.set(TRowIndex+row, TColumnIndex+column, value);
	}
	
	template<typename TMatrix>
	SubMatrix& operator=(const TMatrix& matrix){
		return copyMatrix(matrix, *this);
	}
	
};

#define createSubMatrix(M, TRowIndex, TColumnIndex, TRowCount, TColumnCount) SubMatrix<TRowIndex, TColumnIndex, TRowCount, TColumnCount, decltype(M)>(M)

//! A representation of a part of a parent matrix with deleted row i and deleted column j by referencing it and translating the indices
template <typename TParentMatrix>
class SubMatrixIJ{

	protected:
	
	uint32_t i;
	uint32_t j;
	
	void translateIndices(uint32_t& row, uint32_t& column) const{
		if(row>=i){row++;}
		if(column>=j){column++;}
	}

	public:
	
	typedef typename TParentMatrix::Scalar Scalar;
	static constexpr uint32_t rowCount = TParentMatrix::rowCount-1;
	static constexpr uint32_t columnCount = TParentMatrix::columnCount-1;
	static constexpr bool isMatrix = TParentMatrix::isMatrix;
	
	TParentMatrix& parentMatrix;
	
	SubMatrixIJ(TParentMatrix& parentMatrix, uint32_t i, uint32_t j):i(i),j(j),parentMatrix(parentMatrix){
		static_assert(isMatrix);
	}
	
	const Scalar& get(uint32_t row, uint32_t column = 0) const{
		translateIndices(row, column);
		return parentMatrix.get(row, column);
	}
	
	Scalar& get(uint32_t row, uint32_t column = 0){
		translateIndices(row, column);
		return parentMatrix.get(row, column);
	}
	
	void set(uint32_t row, uint32_t column, Scalar value){
		translateIndices(row, column);
		parentMatrix.set(row, column, value);
	}
	
	template<typename TMatrix>
	SubMatrixIJ& operator=(const TMatrix& matrix){
		return copyMatrix(matrix, *this);
	}
	
};

#define createSubMatrixIJ(M, I, J) SubMatrixIJ<decltype(M)>(M, I, J)

template<uint32_t TSize, typename TScalar>
using Vector = Matrix<TSize, 1, TScalar>;

template<typename TScalar>
using Vector2D = Vector<2, TScalar>;

template<typename TScalar>
using Vector3D = Vector<3, TScalar>;

//! gives the type which results from automatic type conversions when calculating with the scalars of two matrices
template <typename TMatrixA, typename TMatrixB>
struct ResultScalar{using type = decltype((typename TMatrixA::Scalar)1*(typename TMatrixB::Scalar)1);};

//! multiply two matrices
template <typename TMatrixA, typename TMatrixB>
Matrix<TMatrixA::rowCount, TMatrixB::columnCount, typename ResultScalar<TMatrixA,TMatrixB>::type> operator*(const TMatrixA& a, const TMatrixB& b){
	static_assert(TMatrixA::columnCount==TMatrixB::rowCount);//check compatibility to make runtime errors to compile time errors
	Matrix<TMatrixA::rowCount, TMatrixB::columnCount, typename ResultScalar<TMatrixA,TMatrixB>::type> res;
	for(uint32_t inColumn=0; inColumn<TMatrixB::columnCount; inColumn++){
		for(uint32_t row=0; row<TMatrixA::rowCount; row++){
			typename ResultScalar<TMatrixA,TMatrixB>::type dotProduct = 0;
			for(uint32_t column=0; column<TMatrixA::columnCount; column++){
				dotProduct += a.get(row,column)*b.get(column, inColumn);
			}
			res.set(row, inColumn, dotProduct);
		}
	}
	return res;
}

//! visit all elements of a matrix
template <typename Matrix>
void visitMatrix(Matrix& matrix, const std::function<void(uint32_t,uint32_t,typename Matrix::Scalar&)>& f){
	for(uint32_t i=0; i<Matrix::rowCount; i++){
		for(uint32_t j=0; j<Matrix::columnCount; j++){
			f(i,j,matrix.get(i,j));
		}
	}
}

//! Copy Matrices and returns the target
template <typename TMatrixA, typename TMatrixB>
TMatrixB& copyMatrix(const TMatrixA& source, TMatrixB& target){
	static_assert(TMatrixA::rowCount==TMatrixB::rowCount && TMatrixA::columnCount==TMatrixB::columnCount);
	visitMatrix(const_cast<TMatrixA&>(source), [&target](uint32_t row, uint32_t column, typename TMatrixA::Scalar& value){target.set(row, column, value);});
	return target;
}

//! Output Matrices
template <typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
std::ostream& operator<<(std::ostream &out, const TMatrix& m){
	visitMatrix(const_cast<TMatrix&>(m), [&out](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){
		out << std::setw(15) << std::right << value;
		if(row!=TMatrix::rowCount-1 && column==TMatrix::columnCount-1){out << "\n";}
	});
	return out; 
}

//! Subtract Matrices
template <typename TMatrixA, typename TMatrixB, typename std::enable_if<TMatrixA::isMatrix&&TMatrixB::isMatrix, int>::type = 0>
Matrix<TMatrixA::rowCount, TMatrixA::columnCount, typename ResultScalar<TMatrixA, TMatrixB>::type> operator-(const TMatrixA& a, const TMatrixB& b){
	static_assert(TMatrixA::rowCount==TMatrixB::rowCount && TMatrixA::columnCount==TMatrixB::columnCount);
	using ResScalar = typename ResultScalar<TMatrixA, TMatrixB>::type;
	Matrix<TMatrixA::rowCount, TMatrixA::columnCount, ResScalar> res;
	visitMatrix(res, [&a, &b](uint32_t row, uint32_t column, ResScalar& value){value = a.get(row, column)-b.get(row, column);});
	return res;
}

//! Subtract Matrix from zero
template <typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> operator-(const TMatrix& m){
	Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> res;
	visitMatrix(res, [&m](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){value = -m.get(row, column);});
	return res;
}

//! Add Matrices
template <typename TMatrixA, typename TMatrixB, typename std::enable_if<TMatrixA::isMatrix&&TMatrixB::isMatrix, int>::type = 0>
Matrix<TMatrixA::rowCount, TMatrixA::columnCount, typename ResultScalar<TMatrixA, TMatrixB>::type> operator+(const TMatrixA& a, const TMatrixB& b){
	static_assert(TMatrixA::rowCount==TMatrixB::rowCount && TMatrixA::columnCount==TMatrixB::columnCount);
	using ResScalar = typename ResultScalar<TMatrixA, TMatrixB>::type;
	Matrix<TMatrixA::rowCount, TMatrixA::columnCount, ResScalar> res;
	visitMatrix(res, [&a, &b](uint32_t row, uint32_t column, ResScalar& value){value = a.get(row, column)+b.get(row, column);});
	return res;
}

//! Multiply with Scalar
template <typename TMatrix, typename TScalar, typename std::enable_if<TMatrix::isMatrix&&std::is_same<typename TMatrix::Scalar,TScalar>::value, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> operator*(const TMatrix& m, TScalar s){
	Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> res;
	visitMatrix(res, [&m,s](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){value = s*m.get(row, column);});
	return res;
}

//! Multiply with Scalar #2
template <typename TMatrix, typename TScalar, typename std::enable_if<TMatrix::isMatrix&&std::is_same<typename TMatrix::Scalar,TScalar>::value, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> operator*(TScalar s, const TMatrix& m){
	return m*s;
}

//! Divide by Scalar, Attention: In case of integral scalar types this will be integer division!!!
template <typename TMatrix, typename TScalar, typename std::enable_if<TMatrix::isMatrix&&std::is_same<typename TMatrix::Scalar,TScalar>::value, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> operator/(const TMatrix& m, TScalar s){
	Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> res;
	visitMatrix(res, [&m,s](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){value = m.get(row, column)/s;});
	return res;
}

//! Linear Interpolation, time specifices the progress of the interpolaton form start (0) to end (1)
template <typename TMatrixA, typename TMatrixB, typename std::enable_if<TMatrixA::isMatrix&&TMatrixB::isMatrix, int>::type = 0>
Matrix<TMatrixA::rowCount, TMatrixA::columnCount, typename ResultScalar<TMatrixA, TMatrixB>::type> lerp(const TMatrixA& start, const TMatrixB& end, typename ResultScalar<TMatrixA, TMatrixB>::type time){
	static_assert(TMatrixA::rowCount==TMatrixB::rowCount && TMatrixA::columnCount==TMatrixB::columnCount);
	return start*(1-time)+end*time;
}

template <typename TMatrixA, typename TMatrixB>
bool areMatricesEqual(const TMatrixA& a, const TMatrixB& b, typename ResultScalar<TMatrixA, TMatrixB>::type eps = 0){
	static_assert(TMatrixA::columnCount==TMatrixB::columnCount && TMatrixA::rowCount==TMatrixB::rowCount);//check compatibility to make runtime errors to compile time errors
	for(uint32_t i=0; i<TMatrixA::rowCount; i++){
		for(uint32_t j=0; j<TMatrixA::columnCount; j++){
			auto delta = a.get(i,j)-b.get(i,j);
			if(delta<-eps || delta>eps){return false;}
		}
	}
	return true;
}

//! Convert a matrix by casting it's scalars
template<typename TNewScalar, typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, TNewScalar> convertMatrix(const TMatrix& m){
	Matrix<TMatrix::rowCount, TMatrix::columnCount, TNewScalar> res;
	visitMatrix(res, [&m](uint32_t row, uint32_t column, TNewScalar& value){value = static_cast<TNewScalar>(m.get(row, column));});
	return res;
}

//! Type trait to check wether it is a conversion from floating point to integral types
template<typename TSourceScalar, typename TTargetScalar>
using is_floating_point_to_integral = std::integral_constant<bool, std::is_floating_point<TSourceScalar>::value && std::is_integral<TTargetScalar>::value>;

//! round TSourceScalar to TTargetScalar
template <class TSourceScalar, class TTargetScalar, typename std::enable_if<is_floating_point_to_integral<TSourceScalar, TTargetScalar>::value, int>::type = 0>
TTargetScalar rdScalar(TSourceScalar in){
	bool sign = in<(TSourceScalar)0;
	in = sign?-in:in;//abs
	TTargetScalar out = (TTargetScalar)(in+(TSourceScalar)0.5);
	return sign?-out:out;
}

//! simple cast in case it is not a conversion from floating point to integral type
template <class TSourceScalar, class TTargetScalar, typename std::enable_if<!is_floating_point_to_integral<TSourceScalar, TTargetScalar>::value, int>::type = 0>
TTargetScalar rdScalar(TSourceScalar in){
	return static_cast<TTargetScalar>(in);
}

//! Round a matrix if applicable
template<typename TNewScalar, typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, TNewScalar> rdMatrix(const TMatrix& m){
	Matrix<TMatrix::rowCount, TMatrix::columnCount, TNewScalar> res;
	visitMatrix(res, [&m](uint32_t row, uint32_t column, TNewScalar& value){value = rdScalar<typename TMatrix::Scalar,TNewScalar>(m.get(row,column));});
	return res;
}

//! Type Trait to check if it is a square matrix
template<typename TMatrix>
using is_square_matrix = std::integral_constant<bool, TMatrix::rowCount==TMatrix::columnCount>;

//! Calculates the cofactor matrix for a square matrix
template<typename TMatrix, typename std::enable_if<is_square_matrix<TMatrix>::value, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> calcCofactorMatrix(const TMatrix& m){
	Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> res;
	visitMatrix(res, [&m](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){
		typename TMatrix::Scalar d = calcDeterminant(SubMatrixIJ<TMatrix>(const_cast<TMatrix&>(m), row, column));
		bool isNegative = (row+column)%2;
		value = (isNegative?-d:d);
	});
	return res;
}

//! Calculates the inverse of a square matrix using Cramer's rule.
//! WARNING: Inefficient (O(n*n!) for nxn matrix)! For large matrices use sth. like LR decomposition!!!
template<typename TMatrix, typename std::enable_if<is_square_matrix<TMatrix>::value, int>::type = 0>
Matrix<TMatrix::rowCount, TMatrix::columnCount, typename TMatrix::Scalar> calcInverse(const TMatrix& m){
	static_assert(std::is_floating_point<typename TMatrix::Scalar>::value);
	typedef typename TMatrix::Scalar Scalar;
	auto c = calcCofactorMatrix(m);
	Scalar invDet = (((Scalar)1)/calcDeterminant(m));
	return invDet * createTransposeMatrix(c);
};

//! determinant for a 1x1 matrix
template<typename TMatrix, typename std::enable_if<is_square_matrix<TMatrix>::value&&TMatrix::rowCount==1, int>::type = 0>
typename TMatrix::Scalar calcDeterminant(const TMatrix& m){
	return m.get(0,0);
}

//! determinant for a 2x2 matrix
template<typename TMatrix, typename std::enable_if<is_square_matrix<TMatrix>::value&&TMatrix::rowCount==2, int>::type = 0>
typename TMatrix::Scalar calcDeterminant(const TMatrix& m){
	return m.get(0,0)*m.get(1,1)-m.get(0,1)*m.get(1,0);
}

//! determinant for a 3x3 matrix
template<typename TMatrix, typename std::enable_if<is_square_matrix<TMatrix>::value&&TMatrix::rowCount==3, int>::type = 0>
typename TMatrix::Scalar calcDeterminant(const TMatrix& m){
	return m.get(0,0)*m.get(1,1)*m.get(2,2) + m.get(0,1)*m.get(1,2)*m.get(2,0) + m.get(0,2)*m.get(1,0)*m.get(2,1) - m.get(0,2)*m.get(1,1)*m.get(2,0) - m.get(0,1)*m.get(1,0)*m.get(2,2) - m.get(0,0)*m.get(1,2)*m.get(2,1);
}

//! determinant for a nxn matrix
//! WARNING: Inefficient (O(n!) for nxn matrix)! For large matrices use sth. like LR decomposition!!!
template<typename TMatrix, typename std::enable_if<is_square_matrix<TMatrix>::value&&(TMatrix::rowCount>3), int>::type = 0>
typename TMatrix::Scalar calcDeterminant(const TMatrix& m){
	typename TMatrix::Scalar res = 0;
	for(uint32_t column=0; column<TMatrix::columnCount; column++){
		bool isNegative = column%2;
		res += (isNegative?-m.get(0,column):m.get(0,column))*calcDeterminant(SubMatrixIJ<TMatrix>(const_cast<TMatrix&>(m), 0, column));
	}
	return res;
}

//! returns the minimum value from a matrix
template<typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
typename TMatrix::Scalar getMatrixMin(const TMatrix& m){
	typename TMatrix::Scalar res = m.get(0);
	visitMatrix(const_cast<TMatrix&>(m), [&res](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){if(value<res){res = value;}});
	return res;
}

//! returns the maximum value from a matrix
template<typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
typename TMatrix::Scalar getMatrixMax(const TMatrix& m){
	typename TMatrix::Scalar res = m.get(0);
	visitMatrix(const_cast<TMatrix&>(m), [&res](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){if(value>res){res = value;}});
	return res;
}

//! simple square sum matrix norm (doesn't require floating point numbers)
template<typename TMatrix, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
typename TMatrix::Scalar calcSquareSumNorm(const TMatrix& m){
	typename TMatrix::Scalar res = 0;
	visitMatrix(const_cast<TMatrix&>(m), [&res](uint32_t row, uint32_t column, typename TMatrix::Scalar& value){res += value*value;});
	return res;
}

//! The Frobenius Norm is the root of the sum of the squares of the elements (like euklidean vector norm)
template<typename TMatrix, typename TFloat = typename TMatrix::Scalar, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
TFloat calcFrobeniusNorm(const TMatrix& m){
	static_assert(std::is_floating_point<TFloat>::value);
	return (TFloat)sqrt(calcSquareSumNorm(m));
}

//! Normalize the matrix using the frobenius norm
template<typename TMatrix, typename TFloat = typename TMatrix::Scalar, typename std::enable_if<TMatrix::isMatrix, int>::type = 0>
TMatrix& normalize(TMatrix& m){
	TFloat factor = calcFrobeniusNorm<TMatrix,TFloat>(m);
	factor = ((TFloat)1)/factor;
	return factor * m;
}

//! Calculate dot Product of Vectors
template<typename TVectorA, typename TVectorB, typename std::enable_if<TVectorA::isMatrix&&TVectorA::columnCount==1&&TVectorB::isMatrix&&TVectorB::columnCount==1&&TVectorA::rowCount==TVectorB::rowCount, int>::type = 0>
typename ResultScalar<TVectorA,TVectorB>::type calcDotProduct(const TVectorA& a, const TVectorB& b){
	typename ResultScalar<TVectorA,TVectorB>::type res = 0;
	for(uint32_t i=0; i<TVectorA::rowCount; i++){res += a.get(i)*b.get(i);}
	return res;
}

#endif
