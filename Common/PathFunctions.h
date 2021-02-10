#ifndef PATH_FUNCTIONS_H_INCLUDED
#define PATH_FUNCTIONS_H_INCLUDED

#include <StringHelpers.h>

#include <cstdint>
#include <string>

//! returns the path of a folder of a file path with trailing delimeter
template <typename T>
std::basic_string<T> getFolderPath(const std::basic_string<T>& filePath){
	T slash = (T)'/';
	T backslash = (T)'\\';
	for(int32_t i=filePath.size()-1; i>=0; i--){
		T c = filePath[i];
		if(c==slash || c==backslash){
			return filePath.substr(0, i+1);
		}
	}
	std::basic_string<T> res(1, (T)'.');
	#if _WIN32
	res.append(1, (T)'\\');
	#else
	res.append(1, (T)'/');
	#endif
	return res;
}

//! Appends a path delimeter to the path if it is missing and returns the new path
template <typename T>
std::basic_string<T> appendMissingPathDelimeter(const std::basic_string<T>& filePath){
	#ifndef _WIN32
	T pathDelimeter = (T)'/';
	#else
	T pathDelimeter = (T)'\\';
	#endif
	if(filePath.empty()){
		return filePath + pathDelimeter;
	}else{
		T c = filePath[filePath.size()-1];
		if(c!=pathDelimeter && c!=(T)'/'){
			return filePath + pathDelimeter;
		}
	}
	return filePath;
}

//! Returns true if the given path is an absolute path
template <typename T>
bool isAbsolutePath(const std::basic_string<T>& filePath){
	#if _WIN32
	if(filePath.size()<=1){
		return false;
	}else{
		return filePath[1]==(T)':';
	}
	#else
	if(filePath.empty()){
		return false;
	}else{
		return filePath[0] == (T)'/';
	}
	#endif
}

#endif
