#include "utf8.h"

using namespace UTF8Conversion;

std::wstring convertUtf8ToWString(const std::string& utf8String){
	uint32_t size = utf8String.size()+1;
	wchar_t* str = new wchar_t[size];
	utf8ToWchar(utf8String.c_str(), str, size*sizeof(wchar_t));
	std::wstring out(str);
	delete[] str;
	return out;
}

std::string convertWStringToUtf8String(const std::wstring& str){
	uint32_t size = str.size()*sizeof(wchar_t)+1;//upper limit
	char* out = new char[size];
	wcharToUtf8(str.c_str(), out, size);
	std::string outStr(out);
	delete[] out;
	return outStr;
}
