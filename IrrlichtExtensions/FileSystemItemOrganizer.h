#ifndef FileSystemItemOrganizer_H_INCLUDED
#define FileSystemItemOrganizer_H_INCLUDED

#include "IItemOrganizer.h"
#include "ForwardDeclarations.h"

class FileSystemItemOrganizer : public IItemOrganizer{

	private:
	
	irr::io::IFileSystem* fsys;
	
	std::vector<std::wstring> fieldLangKeys;
	std::vector<std::wstring> fieldLabels;
	
	std::vector<IItemOrganizer::Item> content;
	std::vector<uint64_t> fileSizes;
	
	std::string wd;
	
	std::vector<std::wstring> placeLangKeys;
	std::vector<IItemOrganizer::Place> places;
	
	const ILanguagePhrases* lang;
	
	void updateContent();
	
	public:
	
	FileSystemItemOrganizer(irr::io::IFileSystem* fsys);
	
	virtual bool doesExist(const std::string& path) const;
	
	virtual std::string getAbsolutePath(const std::string& relativePath) const;
	
	virtual void setLanguage(const ILanguagePhrases* phrases);
	
	virtual const std::vector<std::wstring>& getItemFieldLabels() const;
	
	virtual bool cd(const std::string& path);
	
	virtual bool mkdir(const std::string& path);
	
	virtual const std::string& pwd() const;
	
	virtual std::vector<IItemOrganizer::Item*> ls(uint32_t fieldIndexForSorting = 0, bool sortAscending = true);
	
	virtual const std::vector<Place>& getPlaces() const;
	
};

#endif
