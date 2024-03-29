#ifndef EFFICIENTLOWPASSFILTER_H_INCLUDED
#define EFFICIENTLOWPASSFILTER_H_INCLUDED

#include <cmath>

//! TSample can be a vector class implementing operator* with scalars or a scalar; TTime must be a scalar
template<typename TSample, typename TTime>
class EfficientLowPassFilter{

	protected:
	
	TSample value;
	TTime lastTime;
	bool init;
	double b;//exponential factor
	TTime minSamplePeriod;
	
	public:
	
	//! The low pass filter is set up that after dT the weight of the new sample is weightAfterDT (continuous version of oldSample*weightAfterDT+newSample*(1-weightAfterDT) after each dT)
	//! minSamplePeriod should be small but large enough to avoid rounding errors.
	EfficientLowPassFilter(double weightAfterDT, TTime dT, TTime minSamplePeriod, TSample unitializedSample = static_cast<TSample>(0)){
		setWeightParameters(weightAfterDT, dT, minSamplePeriod);
		reset(unitializedSample);
	}
	
	virtual ~EfficientLowPassFilter(){}
	
	virtual void setWeightParameters(double weightAfterDT, TTime dT, TTime minSamplePeriod){
		b = -log(weightAfterDT)/static_cast<double>(dT);
		this->minSamplePeriod = minSamplePeriod;
	}
	
	virtual void reset(TSample unitializedSample = static_cast<TSample>(0)){
		init = false;
		value = unitializedSample;
	}	
	
	//! Updates the value of the low pass filter and returns it
	virtual const TSample& update(const TSample& newSample, TTime newTime){
		if(init){
			TTime dt = newTime-lastTime;
			if(dt>minSamplePeriod){
				double f = exp(-b*static_cast<double>(dt));
				value = f*value + (1.0-f)*newSample;
				lastTime = newTime;
			}
		}else{
			value = newSample;
			lastTime = newTime;
			init = true;
		}
		return value;
	}
	
	virtual const TSample& getValue() const{
		return value;
	}
	
	virtual bool isInitialized() const{
		return init;
	}
	
};

#endif
