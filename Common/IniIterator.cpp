#include <cstring>
#include <string>
#include <map>

#include "IniIterator.h"
#include "IniFile.h"

IniIterator::IniIterator(const std::map< std::string, std::map<std::string, std::string> >* data){
	dataptr =  data;
	sectionIt = dataptr->begin();
	if(sectionIt != dataptr->end()){keyIt = sectionIt->second.begin();}
}

bool IniIterator::isSectionAvail() const{
	return sectionIt != dataptr->end();
}

bool IniIterator::isValueAvail() const{
	if(!isSectionAvail()){return false;}
	return keyIt != sectionIt->second.end();
}

void IniIterator::gotoNextSection(){
	if(sectionIt!=dataptr->end()){
		++sectionIt;
		if(sectionIt!=dataptr->end()){
			keyIt = sectionIt->second.begin();
		}
	}
}

void IniIterator::gotoNextValue(){
	++keyIt;
}

const std::string& IniIterator::getCurrentSection() const{
	return sectionIt->first;
}
	
const std::string& IniIterator::getCurrentKey() const{
	return keyIt->first;
}

const std::string& IniIterator::getCurrentValue() const{
	return keyIt->second;
}

