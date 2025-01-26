#ifndef UTF8CfgParser_H_INCLUDED
#define UTF8CfgParser_H_INCLUDED

#include <list>
#include <vector>
#include <string>
#include <cstdint>

#include "ForwardDeclarations.h"

//! Syntax: S -> V;|V;S   V -> V,V|<value>
//! Escape Character: \ (\\ Baslash, \n New line, \, Comma, \; Semicolon, \<any char> Any Char)
//! New lines, leading whitespaces and carriage returns are ignored.
class UnicodeCfgParser{

	private:
	
	uint32_t tokenAmountPerExpression;
	
	std::list<std::vector<std::wstring>> lines;

	public:
	
	UnicodeCfgParser(uint32_t tokenAmountPerExpression);
	
	void parse(const std::wstring& str);
	
	//! returns true if successful
	bool parseFromUTF8File(irr::io::IFileSystem* fsys, const char* fileName);
	
	const std::list<std::vector<std::wstring>>& getResult() const;

};

//! escapes \ \n , ;
std::wstring escapeForUnicodeCfgParser(const std::wstring& toEscape);

#endif
