#ifndef ConstantLanguagePhrases_H_INCLUDED
#define ConstantLanguagePhrases_H_INCLUDED

#include "ILanguagePhrases.h"

#include <unordered_map>
#include <initializer_list>

#include "ForwardDeclarations.h"

//! useful e.g. to define default phrases programmatically or to load phrases from a file
class ConstantLanguagePhrases : public ILanguagePhrases{

	private:
	
	std::unordered_map<std::wstring, std::list<std::wstring> > phrases;
	
	void insertPhrase(const std::wstring& key, const std::wstring& phrase);
	
	public:
	
	ConstantLanguagePhrases(const std::initializer_list<std::pair<std::wstring, std::wstring>>& initialPhrases);

	//! format every %s inside the phrase gets replaced the corresponding replacement in the replacements list (% is an escape char where in all other cases %<any char> means <any char>)
	ConstantLanguagePhrases(irr::io::IFileSystem* fsys, const char* path);
	
	std::wstring getPhrase(const std::wstring& key, const std::list<std::wstring>& replacements, const ILanguagePhrases* defaultPhrases) const;
	
	const std::wstring& getPhrase(const std::wstring& key, const ILanguagePhrases* defaultPhrases) const;
	
};

#endif
