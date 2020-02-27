#ifndef ILANGUAGEPHRASES_H_INCLUDED
#define ILANGUAGEPHRASES_H_INCLUDED

#include <string>
#include <list>

class ILanguagePhrases{
	
	public:
	
	virtual ~ILanguagePhrases(){}
	
	//! returns a phrase for a given key; keys should be prefixed to avoid collisions with other dialogs
	//! a default phrase is returned if no phrase for the key is found, if no default phrase is there the empty string is returned.
	virtual std::wstring getPhrase(const std::wstring& key, const std::list<std::wstring>& replacements, const ILanguagePhrases* defaultPhrases = NULL) const = 0;
	
	//! same as above but without replacements, should be more efficient (depending on the implementation)
	virtual const std::wstring& getPhrase(const std::wstring& key, const ILanguagePhrases* defaultPhrases = NULL) const = 0;

};

#endif
