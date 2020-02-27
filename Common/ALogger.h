#ifndef ALOGGER_H_INCLUDED
#define ALOGGER_H_INCLUDED

template <typename LoggingFunction>
class ALogger{

	protected:
	
	int levelCount;
	bool* shouldLog;
	bool loggingEnabled;
	
	LoggingFunction& loggingFunction;
	
	public:
	
	ALogger(int levelCount, LoggingFunction& loggingFunction):levelCount(levelCount),loggingFunction(loggingFunction){
		shouldLog = new bool[levelCount];
		for(int i=0; i<levelCount; i++){
			shouldLog[i] = true;
		}
		loggingEnabled = true;
	}
	
	virtual ~ALogger(){
		delete[] shouldLog;
	}
	
	//! log some value
	template<typename T>
	ALogger& operator<<(const T& value){
		if(loggingEnabled){loggingFunction.log(value);}
		return *this;
	}
	
	//! specify the current logging level
	void applyCurrentLoggingLevel(int level){
		loggingEnabled = shouldLog[level];
	}
	
	//! enable or disable a specific logging level
	void setLogging(int level, bool enabled){
		shouldLog[level] = enabled;
	}
	
	//! check if a specific logging level is enabled
	bool getLogging(int level) const{
		return shouldLog[level];
	}
	
	//! enable logging for all levels equal or above the specified level and disable it for all the others
	void enableLoggingAbove(int level){
		for(int i=0; i<levelCount; i++){
			shouldLog[i] = i>=level;
		}
	}

};

#endif
