#ifndef INIITERATOR_H_INCLUDED
#define INIITERATOR_H_INCLUDED

#include <string>
#include <map>

//! Class for iterating through inis
class IniIterator{
	public:
	const std::map< std::string, std::map<std::string, std::string> >* dataptr;
	std::map< std::string, std::map<std::string, std::string> >::const_iterator sectionIt;
	std::map<std::string, std::string>::const_iterator keyIt;
	
	IniIterator(const std::map< std::string, std::map<std::string, std::string> >* data);

	bool isSectionAvail() const;

	bool isValueAvail() const;

	//! call only if section available (isSectionAvail)
	void gotoNextSection();

	//! call only if section available (isSectionAvail)
	void gotoNextValue();

	//! call only if section available (isSectionAvail)
	const std::string& getCurrentSection() const;
	
	//! call only if section and key/value available
	const std::string& getCurrentKey() const;

	//! call only if section and key/value available
	const std::string& getCurrentValue() const;
	
};

#endif
