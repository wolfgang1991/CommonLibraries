#ifndef AParallelFunction_H_INCLUDED
#define AParallelFunction_H_INCLUDED

#include <Threading.h>

#include <functional>

//! a general parallel function based on threading function (optional pooling), supports abandoning
class AParallelFunction{

	public:
	
	//! general values with a virtual destructor (can be anything depending on the function)
	class IValue{
		public:
		virtual ~IValue(){}
	};

	protected:
	
	struct FunctionParams{
	
		AParallelFunction* object;
		IValue* input;//function thread only
		Mutex mutex;
		IValue* output;
		bool isAbandoned;
		bool running;
		
		FunctionParams(AParallelFunction* object);
		
		~FunctionParams();
		
		private:
		
		FunctionParams();
		
	};
	
	FunctionParams* params;
	
	double abandonTimeout;
	
	virtual IValue* actualFunction(IValue* input) = 0;
	
	private:
	
	//! the main function which executes actualFunction, deletes the input after function end
	static void* actualFunctionThread(void* data);
	
	public:
	
	//! The abandon timeout specifies how much shall be waited (maximum) if AParallelFunction is deleted. If the thread does not return in this time it is abandoned.
	AParallelFunction(double abandonTimeout);
	
	//! waits for the thread and abandon if timeout
	virtual ~AParallelFunction();
	
	//! true if thread started successful, threadpool optional: a new thread is always created if no thread pool provided
	bool execute(ThreadPool* pool = NULL, IValue* value = NULL);
	
	//! returns the result if finished, NULL means either it hasn't been finished or the result is NULL
	IValue* waitForFinish(double timeout);
	
	//! true if function finished
	bool isFinished();
	
	//! MUST NOT be called if not finished
	//! returns != NULL if finished and result available
	//! the result gets deleted automatically when the AParallelFunction object is deleted or a new result is available (when the thread hangs and needs to be abandoned the result gets deleted on thread exit)
	//! this means a restart of the function must only be triggered if the previous result has been processed and won't be touched
	IValue* getResult();
	
};

//! Implementation to use lambdas
//! !!!WARNING!!!: There seems to be a bug in the lambda capture "by value" implementation. It is possible that the capture becomes memory garbage if the original values go out of scope in parallel (probably gcc bug?). Workaround: Put your captured variables in an anonymous struct and assign it to a volatile local variable in the lambda on the first line.
class LambdaParallelFunction : public AParallelFunction{

	public:
	
	typedef std::function<void()> Function;
	
	protected:
	
	Function f;
	
	AParallelFunction::IValue* actualFunction(AParallelFunction::IValue* input);
	
	public:
	
	LambdaParallelFunction(double abandonTimeout, Function function);
	
};

//! !!!WARNING!!!: There seems to be a bug in the lambda capture "by value" implementation. It is possible that the capture becomes memory garbage if the original values go out of scope in parallel (probably gcc bug?). Workaround: Put your captured variables in an anonymous struct and assign it to a volatile local variable in the lambda on the first line (example implementation: execSocketConnect in UpdateManager.cpp).
//! executes a lambda in a separate thread using LambdaParallelFunction and abandons it if does not finish
//! returns true if it has been finished
//! does not handle any output. If output is required it must be done via the capture.
bool executeLambdaWithTimeout(double timeout, const typename LambdaParallelFunction::Function& function, ThreadPool* pool = NULL);

#endif
