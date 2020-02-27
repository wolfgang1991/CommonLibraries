#ifndef PATH_FUNCTIONS_H_INCLUDED
#define PATH_FUNCTIONS_H_INCLUDED

#include <cstdint>
#include <string>

//! returns the path of a folder of a file path with trailing delimeter
template <typename T>
std::basic_string<T> getFolderPath(const std::basic_string<T>& filePath){
	T pathDelimeter = (T)'/';//TODO windows
	for(int32_t i=filePath.size()-1; i>=0; i--){
		if(filePath[i]==pathDelimeter){
			return filePath.substr(0, i+1);
		}
	}
	return filePath;
}

//! Appends a path delimeter to the path if it is missing and returns the new path
template <typename T>
std::basic_string<T> appendMissingPathDelimeter(const std::basic_string<T>& filePath){
	T pathDelimeter = (T)'/';//TODO windows
	if(filePath.empty()){
		return std::basic_string<T>(filePath)+=pathDelimeter;
	}else if(filePath[filePath.size()-1]!=pathDelimeter){
		return std::basic_string<T>(filePath)+=pathDelimeter;
	}
	return filePath;
}

//! Returns true if the given path is an absolute path
template <typename T>
bool isAbsolutePath(const std::basic_string<T>& filePath){
	if(filePath.empty()){
		return false;
	}else{
		return filePath[0] == (T)'/';//TODO windows
	}
}

#endif
