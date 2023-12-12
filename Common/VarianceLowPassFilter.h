#ifndef VarianceLowPassFilter_H_INCLUDED
#define VarianceLowPassFilter_H_INCLUDED

#include "EfficientLowPassFilter.h"

#include <cmath>

//! TSample can be a vector class implementing operator* with scalars or a scalar; TTime must be a scalar
template<typename TSample, typename TTime>
class VarianceLowPassFilter : public EfficientLowPassFilter<TSample,TTime>{

	protected:
	
	double reactance;
	
	public:
	
	using Super = EfficientLowPassFilter<TSample,TTime>;
	
	//! reactance: how much shall the low pass filter react to large changes (= reduce it's strength)
	VarianceLowPassFilter(double weightAfterDT, TTime dT, TTime minSamplePeriod, double reactance = 100.0, TSample unitializedSample = static_cast<TSample>(0)):
		Super(weightAfterDT, dT, minSamplePeriod, unitializedSample),
		reactance(reactance){}
	
	//! Updates the value of the low pass filter and returns it
	const TSample& update(const TSample& newSample, TTime newTime){
		if(Super::init){
			TTime dt = newTime-Super::lastTime;
			if(dt>Super::minSamplePeriod){
				double f = exp(-Super::b*static_cast<double>(dt));
				double deviation = fabs(0.5*(newSample-Super::value));
				static constexpr double eps = 0.00001;
				double average = fabs(0.5*(newSample+Super::value));
				double f1;
				if(fabs(average)<eps){
					f1 = 1.0-f;
				}else{
					f1 = (1.0-f) * deviation/average * reactance;
				}
				Super::value = (f*Super::value + f1*newSample)/(f+f1);
				Super::lastTime = newTime;
			}
		}else{
			Super::value = newSample;
			Super::lastTime = newTime;
			Super::init = true;
		}
		return Super::value;
	}
	
};

#endif
