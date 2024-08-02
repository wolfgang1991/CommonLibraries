#ifndef ProcedureCallAdapter_H_INCLUDED
#define ProcedureCallAdapter_H_INCLUDED

#include "IRPC.h"

//! Adapter to use lambdas for handling specific remote procedure results
class ProcedureCallAdapter : public IRemoteProcedureCaller{
	
	public:
	
	std::function<void(IRPCValue* results, uint32_t id)> onResult;
	std::function<void(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id)> onError;
	
	ProcedureCallAdapter(const std::function<void(IRPCValue* results, uint32_t id)>& onResult, const std::function<void(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id)>& onError):onResult(onResult),onError(onError){}

	ProcedureCallAdapter(const std::function<void(IRPCValue* results, uint32_t id)>& onResult):onResult(onResult){
		onError = [](int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id){
			delete errorData;
		};
	}
	
	void OnProcedureResult(IRPCValue* results, uint32_t id) override{
		onResult(results, id);
		delete this;
	}
	
	void OnProcedureError(int32_t errorCode, const std::string& errorMessage, IRPCValue* errorData, uint32_t id) override{
		onError(errorCode, errorMessage, errorData, id);
		delete this;
	}
	
};

#endif
