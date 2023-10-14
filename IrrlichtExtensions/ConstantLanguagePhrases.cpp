#include "ConstantLanguagePhrases.h"
#include "UnicodeCfgParser.h"

#include <StringHelpers.h>

#include <sstream>
#include <iostream>

static const std::wstring emptyResult(L"");

void ConstantLanguagePhrases::insertPhrase(const std::wstring& key, const std::wstring& phrase){
	std::list<std::wstring>& list = phrases[key];
	list.clear();//just in case
	std::wstringstream ss;
	for(uint32_t i=0; i<phrase.size(); i++){
		wchar_t c = phrase[i];
		if(c==L'%' && i<phrase.size()-1){
			wchar_t lookAhead = phrase[i+1];
			if(lookAhead==L's'){
				list.push_back(ss.str());
				ss.str(L"");
			}else{
				ss << lookAhead;
			}
			i++;
		}else{
			ss << c;
		}
	}
	list.push_back(ss.str());
}

ConstantLanguagePhrases::ConstantLanguagePhrases(const std::initializer_list<std::pair<std::wstring, std::wstring>>& initialPhrases){
	for(auto it = initialPhrases.begin(); it != initialPhrases.end(); ++it){
		insertPhrase(it->first, it->second);
	}
}
	
ConstantLanguagePhrases::ConstantLanguagePhrases(irr::io::IFileSystem* fsys, const char* path){
	UnicodeCfgParser parser(2);
	parser.parseFromUTF8File(fsys, path);
	const std::list<std::vector<std::wstring>>& res = parser.getResult();
	for(auto it = res.begin(); it != res.end(); ++it){
		if(it->size()>=2){
			insertPhrase((*it)[0], (*it)[1]);
		}else{
			std::cerr << "ERROR: Wrong number of arguments (must be 2 for key value pairs)." << std::endl;
			for(const std::wstring& s : *it){std::cerr << convertWStringToUtf8String(s) << " ";}
			std::cerr << std::endl;
		}
	}
}
	
std::wstring ConstantLanguagePhrases::getPhrase(const std::wstring& key, const std::list<std::wstring>& replacements, const ILanguagePhrases* defaultPhrases) const{
	auto it = phrases.find(key);
	if(it==phrases.end()){
		if(defaultPhrases==NULL){
			return key;
		}else{
			return defaultPhrases->getPhrase(key, replacements, NULL);
		}
	}else{
		const std::list<std::wstring>& list = it->second;
		std::wstringstream ss;
		auto rit = replacements.begin();
		for(auto lit = list.begin(); lit!=list.end(); ++lit){
			ss << *lit;
			if(rit != replacements.end()){
				ss << *rit;
				++rit;
			}
		}
		return ss.str();
	}
}

const std::wstring& ConstantLanguagePhrases::getPhrase(const std::wstring& key, const ILanguagePhrases* defaultPhrases) const{
	auto it = phrases.find(key);
	if(it==phrases.end()){
		if(defaultPhrases==NULL){
			return key;
		}else{
			return defaultPhrases->getPhrase(key, NULL);
		}
	}else{
		const std::list<std::wstring>& list = it->second;
		if(list.empty()){
			return emptyResult;
		}else{
			return list.front();
		}
	}
}
