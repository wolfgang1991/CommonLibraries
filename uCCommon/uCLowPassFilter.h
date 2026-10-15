#ifndef uCLowPassFilter_H
#define uCLowPassFilter_H

#include <stdint.h>

//! seconds2TimeUnit: e.g. 1000.0 to convert to ms
template <typename TTime>
constexpr TTime calculateMaxIntegratedTime(double cutoffFrequency, double seconds2TimeUnit) {
	// Steady state (integratedTime == M): weight of new sample alpha = dt / (M + dt).
	// First order RC filter: alpha = dt / (RC + dt), RC = 1 / (2*pi*fc)  =>  M = RC.
	// cutoffFrequency must be given in 1/(unit of TTime).
	return static_cast<TTime>(seconds2TimeUnit / (2.0 * 3.14159265358979323846 * cutoffFrequency) + 0.5);
}

//! Low pass filter also usable with integers (e.g. 8 bit MCUs) based on averaging over time with limited weight for previously integrated values
template <typename TSample, typename TTime, TTime MaxIntegratedTime, TTime MinDeltaTime = 0>
class uCLowPassFilter {

	private:
	
	TSample value;
	TTime lastTime;
	TTime integratedTime;

	public:

	uCLowPassFilter() {
		reset(0, 0);
	}

	void reset(TSample initialValue, TTime initialTime) {
		value = initialValue;
		lastTime = initialTime;
		integratedTime = 0;
	}

	const TSample& update(TSample newValue, TTime currentTime) {
		TTime deltaTime = currentTime - lastTime;
		if(deltaTime>MinDeltaTime){
			lastTime = currentTime;
			value = value * integratedTime + newValue * deltaTime;
			integratedTime += deltaTime;
			value /= integratedTime;
			if(integratedTime > MaxIntegratedTime) {
				integratedTime = MaxIntegratedTime;
			}
		}
		return value;
	}

};

#endif
