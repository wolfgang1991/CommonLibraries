#ifndef UTF8_H_INCLUDED
#define UTF8_H_INCLUDED

#include <cstdint>
#include <string>

std::wstring convertUtf8ToWString(const std::string& utf8String);

std::wstring convertUtf8ToWString(const char* utf8String);

std::wstring convertUtf8ToWString(const char* utf8String, size_t length);

std::string convertWStringToUtf8String(const wchar_t* str, size_t length);

std::string convertWStringToUtf8String(const wchar_t* str);

std::string convertWStringToUtf8String(const std::wstring& str);

uint32_t utf8codepoint(const char **_str);

void utf8fromcodepoint(uint32_t cp, char **_dst, uint64_t *_len);

#endif
