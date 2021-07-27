#include <UnicodeCfgParser.h>
#include <StringHelpers.h>
#include <utf8.h>

#include <irrString.h>
#include <IReadFile.h>
#include <IFileSystem.h>

#include <sstream>
#include <iostream>

using namespace irr;
using namespace io;

UnicodeCfgParser::UnicodeCfgParser(uint32_t tokenAmountPerExpression):tokenAmountPerExpression(tokenAmountPerExpression){}

void UnicodeCfgParser::parse(const std::wstring& str){
	lines.clear();
	if(str.empty()){return;}
	lines.push_back(std::vector<std::wstring>(tokenAmountPerExpression));
	std::vector<std::wstring>* line = &lines.back();
	std::wstringstream token;
	uint32_t tokenIndex = 0;
	bool isEscape = false;
	bool isLeadingChar = true;
	for(uint32_t i=0; i<str.size(); i++){
		wchar_t c = str[i];
		if(isEscape){
			if(c==L'n'){
				token << L'\n';
			}else{
				token << c;
			}
			isEscape = false;
		}else if(c==L',' || c==L';'){
			if(tokenIndex<line->size()){
				(*line)[tokenIndex] = token.str();
			}else{
				std::cerr << "Parser Error: Too many tokens in line: ";
				for(uint32_t j=0; j<line->size(); j++){std::cerr << (j>0?",":"") << convertWStringToUtf8String((*line)[j]);}
				std::cerr << std::endl;
			}
			token.str(L"");
			if(c==L';'){
				lines.push_back(std::vector<std::wstring>(tokenAmountPerExpression));
				line = &lines.back();
				tokenIndex = 0;
			}else{
				tokenIndex++;
			}
			isLeadingChar = true;
		}else if(c==L'\\'){
			isEscape = true;
			isLeadingChar = false;
		}else if(c!=L'\n' && c!=L'\r' && ((c!=L' ' && c!=L'\t') || !isLeadingChar)){//ignore LF,CR and leading whitespaces
			token << c;
			isLeadingChar = false;
		}
	}
	if(tokenIndex==0 && (*line)[0].empty()){
		lines.pop_back();
	}
}

const std::list<std::vector<std::wstring>>& UnicodeCfgParser::getResult() const{
	return lines;
}

bool UnicodeCfgParser::parseFromUTF8File(irr::io::IFileSystem* fsys, const char* fileName){
	IReadFile* rf = fsys->createAndOpenFile(fileName);//use ireadfile for asset support (android)
	if(rf){
		uint32_t size = rf->getSize();
		char* buffer = new char[size+1];
		rf->read(buffer, size);
		buffer[size] = '\0';
		rf->drop();
		wchar_t* str = new wchar_t[size+1];
      UTF8Conversion::utf8ToWchar(buffer, str, (irr::u64)((size+1)*sizeof(wchar_t)));
		parse(str);
		delete[] str;
		delete[] buffer;
		return true;
	}
	return false;
}
