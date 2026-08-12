#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <timing.h>

#include <functional>
#include <string>
#include <ostream>
#include <cstdint>

std::ostream& hex_dump(std::ostream& os, const void *buffer, std::size_t bufsize, bool showPrintableChars = true);

void runForAllFiles(const std::string& path, const std::function<void(const std::string&)>& f, bool recursive = false);

//! use square and multiply to raise value to an integer power
double powSAM(double value, uint32_t power);

//! mapping must be bijective
template<typename TMapIn, typename TMapOut>
TMapOut invertMap(const TMapIn& m){
	TMapOut res;
	for(auto it = m.begin(); it != m.end(); ++it){
		res[it->second] = it->first;
	}
	return res;
}

//! TReverseMap could be a std::map<TValue, size_t> or std::unordered_map<TValue, size_t>
//! TVector could be a std::vector<TValue> or std::list<TValue>, std::array etc. as long as it supports iterators
template <typename TVector, typename TReverseMap>
TReverseMap reverseVectorMapping(const TVector& ctr){
	TReverseMap res;
	size_t idx = 0;
	for(auto it = ctr.begin(); it != ctr.end(); ++it){
		res[*it] = static_cast<typename TReverseMap::mapped_type>(idx);
		idx++;
	}
	return res;
}

template<typename TMap>
const typename TMap::mapped_type& getWithDefault(const TMap& m, const typename TMap::key_type& key, const typename TMap::mapped_type& defaultValue){
	auto it = m.find(key);
	if(it!=m.end()){
		return it->second;
	}else{
		return defaultValue;
	}
}

//! saves / restores a variable on the current stack (by using a local variable)
#define PUSH_VAR(VARNAME, NEWVALUE) { decltype(VARNAME) savedVariable = VARNAME; VARNAME = NEWVALUE;
#define POP_VAR(VARNAME) VARNAME = savedVariable; }

//! execute every N seconds
#define EXECUTE_EVERY_N_SECONDS(N, CODE) { \
	static double lastTime = 0; \
	double currentTime = getSecs(); \
	if(currentTime - lastTime >= N){ \
		lastTime = currentTime; \
		CODE \
	} \
}

#endif
