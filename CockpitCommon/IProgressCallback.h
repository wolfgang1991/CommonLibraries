#ifndef PROGRESS_CALLBACK_H_INCLUDED
#define PROGRESS_CALLBACK_H_INCLUDED

//! Interface for progress updates (useful in case blocking functions are used concurrently)
class IProgressCallback{

	protected:
	
	float stepSize;

	public:
	
	IProgressCallback(){
		stepSize = 0;
	}
	
	virtual ~IProgressCallback(){}
	
	void setStepSize(float stepSize){
		this->stepSize = stepSize;
	}
	
	virtual void nextStep() = 0;

};

#endif
