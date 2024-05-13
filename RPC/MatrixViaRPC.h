#ifndef MatrixViaRPC_H_
#define MatrixViaRPC_H_

#include <IRPC.h>
#include <Matrix.h>

#include <cassert>

template<typename TMatrix>
IRPCValue* createRPCValueFromMatrix(const TMatrix& value){
	ArrayValue* res = new ArrayValue();
	for(uint32_t i=0; i<value.size; i++){
		res->values.push_back(createRPCValue<typename TMatrix::Scalar>(value[i]));
	}
	return res;
}

template <uint32_t TRowCount, uint32_t TColumnCount, typename TScalar>
IRPCValue* createRPCValue(const Matrix<TRowCount, TColumnCount, TScalar>& m){
	return createRPCValueFromMatrix(m);
}

template <typename T>
IRPCValue* createRPCValue(const TransposeMatrix<T>& m){
	return createRPCValueFromMatrix(m);
}

template <uint32_t TRowIndex, uint32_t TColumnIndex, uint32_t TRowCount, uint32_t TColumnCount, typename TParentMatrix>
IRPCValue* createRPCValue(const SubMatrix<TRowIndex, TColumnIndex, TRowCount, TColumnCount, TParentMatrix>& m){
	return createRPCValueFromMatrix(m);
}

template<typename TNativeValue>
TNativeValue createNativeMatrix(IRPCValue* rpcValue){
	assert(rpcValue->getType()==IRPCValue::ARRAY);
	assert(((ArrayValue*)rpcValue)->values.size()==TNativeValue::size);
	TNativeValue res;
	for(uint32_t i=0; i<res.size; i++){
		res[i] = createNativeValue<typename TNativeValue::Scalar>(((ArrayValue*)rpcValue)->values[i]);
	}
	return res;
}

#define FILL_NATIVE_MATRIX_ALIAS(ALIAS, FIELD_NAME) \
	nativeValue.FIELD_NAME = createNativeMatrix<decltype(nativeValue.FIELD_NAME)>(((ObjectValue*)rpcValue)->values[#ALIAS]);

#define FILL_NATIVE_MATRIX(FIELD_NAME) \
	FILL_NATIVE_MATRIX_ALIAS(FIELD_NAME, FIELD_NAME)

#endif
