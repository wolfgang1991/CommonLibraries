#include <IniFile.h>
#include <IniIterator.h>
#include <IniParser.h>

#include <iostream>
#include <cstring>
#include <cassert>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

static const char characters[] = { '\n', '\\', '#', ';', ' ', '\t', '[', ']', '='};
static const char escapes[] = {'n', '\\', '#', ';', ' ', 't', '0', '1', '2'};
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
				if(i<s.size()){return s;}//assert
				int idx = findChar(escapes, escapeSize, s[i]);
				if(idx<escapeSize){return s;}//assert
				ss << characters[idx];
			}else{
				ss << s[i];
			}
		}else if(escapeChar){
			sthToEscape = true;
			i++;
			if(i<s.size()){return s;}//assert
			int idx = findChar(escapes, escapeSize, s[i]);
			if(idx<escapeSize){return s;}//assert
			ss << s.substr(0, i-1) << characters[idx];
		}
	}
	if(sthToEscape){
		return ss.str();
	}else{
		return s;
	}
}

std::string& IniFile::get(const std::string& section, const std::string& key){
	return data[section][key];
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
		if(cs.size()>0){ss<<"["<<escapeIniStrings(cs)<<"]"<<endl;}
		while(it->isValueAvail()){
			ss << escapeIniStrings(it->getCurrentKey()) << " = " << escapeIniStrings(it->getCurrentValue()) << endl;
			it->gotoNextValue();
		}
		it->gotoNextSection();
		ss << endl;
	}
	delete it;
	return ss.str();
}

bool IniFile::save(const std::string& file) const{
	ofstream fp;
	fp.open(file.c_str());
	if(fp.fail()){return false;}
	fp << toString();
	fp.close();
	if(fp.fail()){return false;}
	return true;
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
	ifstream infile;
	infile.open (file.c_str(), ifstream::in);
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

