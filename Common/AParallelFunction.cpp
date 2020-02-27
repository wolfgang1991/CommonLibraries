#include <AParallelFunction.h>
#include <timing.h>

#include <iostream>

void* AParallelFunction::actualFunctionThread(void* data){
	FunctionParams* params = (FunctionParams*)data;
	IValue* output = params->object->actualFunction(params->input);
	lockMutex(params->mutex);
	bool abandoned = params->isAbandoned;
	params->running = false;
	delete params->output;//in case this function runs for more than one time
	params->output = output;
	unlockMutex(params->mutex);
	if(abandoned){delete params;}
	return NULL;
}

AParallelFunction::AParallelFunction(double abandonTimeout){
	this->abandonTimeout = abandonTimeout;
	params = new FunctionParams(this);
}

AParallelFunction::~AParallelFunction(){
	waitForFinish(abandonTimeout);
	if(!isFinished()){
		lockMutex(params->mutex);
		params->isAbandoned = true;
		unlockMutex(params->mutex);
	}else{
		delete params;
	}
}

AParallelFunction::IValue* AParallelFunction::waitForFinish(double timeout){
	double startTime = getSecs();
	bool timoutOccured = false;
	while(!timoutOccured && !isFinished()){
		delay(1);
		timoutOccured = getSecs()-startTime>timeout;
	}
	if(timoutOccured){
		std::cout << "timoutOccured in parallel function." << std::endl;
		return NULL;
	}else{
		return params->output;
	}
}

bool AParallelFunction::execute(ThreadPool* pool, AParallelFunction::IValue* value){
	if(!isFinished()){
		delete value;
		return false;
	}
	params->running = true;
	delete params->input;//in case this function runs for more than one time
	params->input = value;
	if(pool){
		return pool->startThreadedFunction(AParallelFunction::actualFunctionThread, params)!=NULL;
	}else{
		Thread newThread;
		return createThread(newThread, AParallelFunction::actualFunctionThread, params, false);
	}
}

bool AParallelFunction::isFinished(){
	lockMutex(params->mutex);
	bool finished = !params->running;
	unlockMutex(params->mutex);
	return finished;
}

AParallelFunction::IValue* AParallelFunction::getResult(){
	return params->output;
}

AParallelFunction::FunctionParams::FunctionParams(AParallelFunction* object){
	this->object = object;
	output = input = NULL;
	isAbandoned = running = false;
	initMutex(mutex);
}
		
AParallelFunction::FunctionParams::~FunctionParams(){
	delete output;
	delete input;
	//dont delete the object, this is done externally
	deleteMutex(mutex);
}

AParallelFunction::IValue* LambdaParallelFunction::actualFunction(AParallelFunction::IValue* input){
	f();
	return NULL;
}

LambdaParallelFunction::LambdaParallelFunction(double abandonTimeout, Function function):AParallelFunction(abandonTimeout),f(function){}

bool executeLambdaWithTimeout(double timeout, const typename LambdaParallelFunction::Function& function, ThreadPool* pool){
	LambdaParallelFunction lpf(0.0, function);
	if(lpf.execute(pool, NULL)){
		lpf.waitForFinish(timeout);
		return lpf.isFinished();
	}else{
		std::cerr << "Error executing parallel lambda." << std::endl;
	}
	return false;
}
