#ifndef PATH_FUNCTIONS_H_INCLUDED
#define PATH_FUNCTIONS_H_INCLUDED

#include <StringHelpers.h>

#include <cstdint>
#include <cstring>
#include <string>

#if _WIN32
#include <direct.h>
#define PATH_DELIMETER '\\'
#else
#include <unistd.h>
#define PATH_DELIMETER '/'
#endif

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
	res.append(1, (T)PATH_DELIMETER);
	return res;
}

//! Appends a path delimeter to the path if it is missing and returns the new path
template <typename T>
std::basic_string<T> appendMissingPathDelimeter(const std::basic_string<T>& filePath){
	if(filePath.empty()){
		return filePath + PATH_DELIMETER;
	}else{
		T c = filePath[filePath.size()-1];
		if(c!=PATH_DELIMETER && c!=(T)'/'){
			return filePath + PATH_DELIMETER;
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
