#ifndef SampleBin_H_INCLUDED
#define SampleBin_H_INCLUDED

//! Avaraging of samples inside time difference (binSize) for downsampling of a signal in time domain
template<typename TSample, typename TTime>
class SampleBin{
	
	private:
	
	TTime binSize;
	
	TSample averaged;
	
	TSample averaging;
	double count;
	TTime lastTime;
	
	public:
	
	SampleBin(TTime binSize, const TSample& initialSample = TSample(0), TTime initialTime = TTime(0)){
		count = 0.0;
		averaged = averaging = initialSample;
		this->binSize = binSize;
		lastTime = initialTime;
	}
	
	void addSample(const TSample& sample, TTime time){
		if(time-lastTime>binSize){
			lastTime = time;
			averaged = averaging;
			count = 0.0;
		}
		const double cp1 = count+1.0;
		averaging = averaging*(count/cp1) + sample*(1.0/cp1);
	}
	
	const TSample& getValue() const{
		return averaged;
	}
	
};

#endif
