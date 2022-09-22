#include <IniFile.h>
#include <IniIterator.h>
#include <IniParser.h>
#include <utf8.h>

#include <iostream>
#include <cstring>
#include <cassert>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

static const char characters[] = { '\n', '\\', '#', ';', ' ', '\t', '[', ']', '=', '\r'};
static const char escapes[] = {'n', '\\', '#', ';', ' ', 't', '0', '1', '2', 'r'};
static const int escapeSize = sizeof(escapes)/sizeof(escapes[0]);

static inline int findChar(const char* chars, int size, char toFind){
	for(int i=0; i<size; i++){
		if(chars[i]==toFind){
			return i;
		}
	}
	return size;
}

std::string escapeIniStrings(const std::string& s){
	std::stringstream ss;
	bool sthToEscape = false;
	for(uint32_t i=0; i<s.size(); i++){
		int idx = findChar(characters, escapeSize, s[i]);
		if(sthToEscape){
			if(idx!=escapeSize){
				ss << '\\' << escapes[idx];
			}else{
				ss << s[i];
			}
		}else{
			if(idx!=escapeSize){
				sthToEscape = true;
				ss << s.substr(0, i) << '\\' << escapes[idx];
			}
		}
	}
	if(sthToEscape){
		return ss.str();
	}else{
		return s;
	}
}

std::string unescapeIniStrings(const std::string& s){
	std::stringstream ss;
	bool sthToEscape = false;
	for(uint32_t i=0; i<s.size(); i++){
		bool escapeChar = s[i]=='\\';
		if(sthToEscape){
			if(escapeChar){
				i++;
				if(i>=s.size()){return s;}//assert(i<s.size())
				int idx = findChar(escapes, escapeSize, s[i]);
				if(idx>=escapeSize){return s;}//assert(idx<escapeSize)
				ss << characters[idx];
			}else{
				ss << s[i];
			}
		}else if(escapeChar){
			sthToEscape = true;
			i++;
			if(i>=s.size()){return s;}//assert(i<s.size())
			int idx = findChar(escapes, escapeSize, s[i]);
			if(idx>=escapeSize){return s;}//assert(idx<escapeSize)
			ss << s.substr(0, i-1) << characters[idx];
		}
	}
	if(sthToEscape){
		return ss.str();
	}else{
		return s;
	}
}

const std::string& IniFile::get(const std::string& section, const std::string& key, const std::string& defaultValue) const{
	auto it = data.find(section);
	if(it!=data.end()){
		auto it2 = it->second.find(key);
		if(it2!=it->second.end()){
			return it2->second;
		}
	}
	return defaultValue;
}

const std::string& IniFile::get(const std::string& section, const std::string& key) const{
	static const std::string empty("");
	return get(section, key, empty);
}

const std::string& IniFile::get(std::map< std::string, std::map<std::string, std::string> >::iterator it, const std::string& key, const std::string& defaultValue){
	auto it2 = it->second.find(key);
	if(it2!=it->second.end()){
		return it2->second;
	}
	return defaultValue;
}

void IniFile::set(const std::string& section, const std::string& key, const std::string& value){
	data[section][key] = value;
}

void IniFile::removeSection(const std::string& section){
	data.erase(section);
}

void IniFile::removeValue(const std::string& section, const std::string& key){
	data[section].erase(key);
}

IniIterator* IniFile::createNewIterator() const{
	return new IniIterator(&data);
}

void IniFile::print() const{
	cout << toString();
}

std::string IniFile::toString() const{
	std::stringstream ss;
	IniIterator* it = createNewIterator();
	while(it->isSectionAvail()){
		string cs = it->getCurrentSection();
		if(cs.size()>0){ss<<"["<<escapeIniStrings(cs)<<"]\n";}
		while(it->isValueAvail()){
			ss << escapeIniStrings(it->getCurrentKey()) << " = " << escapeIniStrings(it->getCurrentValue()) << "\n";
			it->gotoNextValue();
		}
		it->gotoNextSection();
		ss << "\n";
	}
	delete it;
	return ss.str();
}

bool IniFile::save(const std::string& file) const{
	ofstream fp(file.c_str(), std::ofstream::binary | std::ofstream::trunc);
	if(fp.good()){
		fp << toString();
		return true;
	}
	return false;
}

bool IniFile::isSectionAvailable(const std::string& section) const{
	std::map< std::string, std::map<std::string, std::string> >::const_iterator it = data.find(section);
	return !(it==data.end());
}

bool IniFile::isAvailable(const std::string& section, const std::string& key) const{
	std::map< std::string, std::map<std::string, std::string> >::const_iterator it = data.find(section);
	if(it==data.end()){return false;}
	std::map<std::string, std::string>::const_iterator itb = it->second.find(key);
	return itb!=it->second.end();
}

IniFile::IniFile(const std::string& file){
	IniParser* p = new IniParser;
	ifstream infile(file.c_str(), ifstream::binary);
	if (infile.is_open()){
		char ch; infile.get(ch);
		while (infile.good()) {
			if(p->doStep(ch)){
				set(unescapeIniStrings(p->curSection), unescapeIniStrings(p->curKey), unescapeIniStrings(p->curValue));
			}
			infile.get(ch);
		}
		if(p->doStep('\n')){//EOF
			set(unescapeIniStrings(p->curSection), unescapeIniStrings(p->curKey), unescapeIniStrings(p->curValue));
		}
		infile.close();
	}
	delete p;
}

void IniFile::setFromString(const char* str){
	IniParser* p = new IniParser;
	int i = 0;
	char c = str[i];
	while(c!='\0'){
		if(p->doStep(c)){
			set(unescapeIniStrings(p->curSection), unescapeIniStrings(p->curKey), unescapeIniStrings(p->curValue));
		}
		i++;
		c = str[i];
	}
	if(p->doStep('\n')){//EOF
		set(unescapeIniStrings(p->curSection), unescapeIniStrings(p->curKey), unescapeIniStrings(p->curValue));
	}
	delete p;
}

void IniFile::moveSection(const std::string& start, const std::string& end){
	data[end] = data[start];
	removeSection(start);
}

const std::map<std::string, std::string>& IniFile::getSection(const std::string& section) const{
	auto it = data.find(section);
	if(it != data.end()){
		return it->second;
	}else{
		static const std::map<std::string, std::string> emptyMap;
		return emptyMap;
	}
}

void IniFile::setSection(const std::string& section, const std::map<std::string, std::string>& key2value){
	data[section] = key2value;
}

void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, const std::string& value){
	section[key] = value;
}

void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, const std::wstring& value){
	section[key] = convertWStringToUtf8String(value);
}

void setToIniSection(std::map<std::string, std::string>& section, const std::string& key, bool value){
	setToIniSection(section, key, (int)value);
}

const std::string& getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, const std::string& defaultValue){
	auto it = section.find(key);
	if(it == section.end()){
		return defaultValue;
	}else{
		return it->second;
	}
}

std::wstring getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, const std::wstring& defaultValue){
	auto it = section.find(key);
	if(it == section.end()){
		return defaultValue;
	}else{
		return convertUtf8ToWString(it->second);
	}
}

bool getFromIniSection(const std::map<std::string, std::string>& section, const std::string& key, bool defaultValue){
	return (bool)getFromIniSection<int>(section, key, (int)defaultValue);
}
