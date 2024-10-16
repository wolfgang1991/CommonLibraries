#ifndef IItemOrganizer_H_INCLUDED
#define IItemOrganizer_H_INCLUDED

#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <locale>
#include <cstdint>

class ILanguagePhrases;

//! Represents a file system like organization of entities, can be used as backend for open/save dialogs as abstraction of underlying storage mechanisms
//! general rule: if a path is not found relative on the current working directory, it is interpreted as absolute path
class IItemOrganizer{
	
	public:
	
	struct Item{
		bool isDirectory;
		std::string relativePath;//! relative to current directory for which this item was created
		std::vector<std::wstring> fields;
		const void* additionalData;//! implementation specific
		uint32_t additionalID;//! implementation specific
	};
	
	struct Place{
		std::wstring indication;
		std::string path;
	};
	
	virtual ~IItemOrganizer(){}
	
	virtual std::string getAbsolutePath(const std::string& relativePath) const = 0;
	
	virtual std::string getAbsolutePath(Item* item) const{
		return getAbsolutePath(item->relativePath);
	}
	
	virtual bool doesExist(const std::string& path) const = 0;
	
	//! returns a vector of usual places
	virtual const std::vector<Place>& getPlaces() const = 0;
	
	//! once a language is set all labels need to be updated, if a required key is not available the default language (English) is used
	//! the required keys depend on the implementation, for example an entity can have different fields which require different translations
	virtual void setLanguage(const ILanguagePhrases* phrases) = 0;
	
	//! returns the labels for each field of the items, depends on the language; each field is uniquely identified by it's index
	virtual const std::vector<std::wstring>& getItemFieldLabels() const = 0;
	
	//! change current working directory, true if successful
	virtual bool cd(const std::string& path) = 0;
	
	//! create a new directory if possible, true if successful
	virtual bool mkdir(const std::string& path) = 0;
	
	//! returns current working directory (path)
	virtual const std::string& pwd() const = 0;
	
	//! returns the content (relative pathes) of the current working directory
	//! items are deleted on cd
	virtual std::vector<Item*> ls(uint32_t fieldIndexForSorting = 0, bool sortAscending = true) = 0;
	
	//! size must be >0. The delimeter at index 0 is the preferred delimeter. However it is legal to use all delimeters in a path.
	virtual const std::vector<char>& getPathDelimeter() const = 0;
	
};

//! creates a function which returns true if f1<f2 according to the field with index fieldIndex
inline std::function<bool(IItemOrganizer::Item* f1, IItemOrganizer::Item* f2)> createFieldLessComparator(uint32_t fieldIndex, bool sortAscending){
	return [fieldIndex,sortAscending](IItemOrganizer::Item* f1, IItemOrganizer::Item* f2){
		if(f1->isDirectory!=f2->isDirectory){return f1->isDirectory;}//directories first
		std::wstring& f1s = f1->fields[fieldIndex];
		std::wstring& f2s = f2->fields[fieldIndex];
		try{
			return sortAscending?(std::locale("").operator()(f1s,f2s)):(std::locale("").operator()(f2s,f1s));
		}catch (const std::runtime_error& e){// empty locale seems not to exist on windows although the standard defines this as the "user preferred locale" which must exist
			try{
				return sortAscending?(std::locale("en_US.UTF-8").operator()(f1s,f2s)):(std::locale("en_US.UTF-8").operator()(f2s,f1s));
			}catch (const std::runtime_error& e){
				return sortAscending?(f1s<f2s):(f2s<f1s);
			}
		}
	};
}

//! TContainer is a container of IItemOrganizer::Item*
template<typename TContainer>
void sortItemsByField(TContainer& ctr, uint32_t fieldIndex, bool sortAscending){
	std::sort(ctr.begin(), ctr.end(), createFieldLessComparator(fieldIndex, sortAscending));
}

#endif
