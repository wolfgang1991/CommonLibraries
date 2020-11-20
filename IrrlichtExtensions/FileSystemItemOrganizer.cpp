#include "FileSystemItemOrganizer.h"
#include "ConstantLanguagePhrases.h"

#include <utf8.h>
#include <StringHelpers.h>

#include <IFileSystem.h>

#include <iomanip>
#include <sstream>
#include <ctime>
#include <iostream>

#ifdef _WIN32
//TODO
#else
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

using namespace irr;
using namespace core;
using namespace io;

static const ConstantLanguagePhrases defaultPhrases({
	{L"File::NAME",L"Name"},
	{L"File::SIZE",L"Size"},
	{L"File::MODIFICATION",L"Modified"},
	{L"Place::HOME",L"Home"},
	{L"Place::ROOT",L"/"}
});

FileSystemItemOrganizer::FileSystemItemOrganizer(irr::io::IFileSystem* fsys):fsys(fsys),fieldLangKeys{L"File::NAME", L"File::SIZE", L"File::MODIFICATION"}{
	#ifdef _WIN32
	//TODO add places: drives, home dir, documents dir
	#elif defined(__ANDROID__)
	//TODO add places: internal/external flash, documents dir
	#elif defined(__APPLE__)
	//TODO add places: app home dir, documents dir
	#else//linux or compatible
		placeLangKeys.push_back(L"Place::HOME");
		const char* homedir;
		if((homedir = getenv("HOME")) == NULL){
			homedir = getpwuid(getuid())->pw_dir;
		}		
		places.push_back(IItemOrganizer::Place{L"", homedir});
		placeLangKeys.push_back(L"Place::ROOT");
		places.push_back(IItemOrganizer::Place{L"", "/"});
		//TODO find and add mount points
	#endif
	setLanguage(&defaultPhrases);
	updateContent();
}

void FileSystemItemOrganizer::updateContent(){
	wd = fsys->getWorkingDirectory().c_str();
	IFileList* l = fsys->createFileList();
	content.clear();
	content.reserve(l->getFileCount());
	fileSizes.clear();
	fileSizes.reserve(l->getFileCount());
	for(u32 i=0; i<l->getFileCount(); i++){
		const path& filename = l->getFileName(i);
		if(filename.size()>0){
			std::string cleanFileName = filename[0]=='/'?&(filename.c_str()[1]):filename.c_str();
			if(cleanFileName!=".." && cleanFileName!="."){//don't include parent and this directory
				uint64_t fileSize = 0;
				time_t modificationTime = 0;
				#ifdef _WIN32
				#error TODO Implement
				#else
				struct stat64 s;
				if(stat64(filename.c_str(), &s)==0){
					modificationTime = s.st_mtim.tv_sec;
					fileSize = s.st_size;
				}else{
					std::cerr << "stat64 error (" << errno << "): " << strerror(errno) << std::endl;
				}
				#endif
				std::cout << "path: " << convertWStringToUtf8String(convertUtf8ToWString(cleanFileName.c_str())) << std::endl;
				struct tm* timeinfo = localtime(&modificationTime);
				std::wstringstream ss; ss << (1900+timeinfo->tm_year) << L"-" << std::setfill(L'0') << std::setw(2) << (1+timeinfo->tm_mon) << L"-" << std::setfill(L'0') << std::setw(2) << timeinfo->tm_mday << " " << std::setfill(L'0') << std::setw(2) << timeinfo->tm_hour << ":" << std::setfill(L'0') << std::setw(2) << timeinfo->tm_min; 
				content.push_back(IItemOrganizer::Item{l->isDirectory(i), cleanFileName.c_str(), {convertUtf8ToWString(cleanFileName.c_str()), l->isDirectory(i)?L"":convertUtf8ToWString(getHumanReadableSpace(fileSize)), ss.str()}, NULL, (uint32_t)content.size()});
				fileSizes.push_back(fileSize);
			}
		}
	}
	l->drop();
}

const std::vector<IItemOrganizer::Place>& FileSystemItemOrganizer::getPlaces() const{
	return places;
}

void FileSystemItemOrganizer::setLanguage(const ILanguagePhrases* phrases){
	fieldLabels.clear();
	fieldLabels.reserve(fieldLangKeys.size());
	for(uint32_t i=0; i<fieldLangKeys.size(); i++){
		fieldLabels.emplace_back(phrases->getPhrase(fieldLangKeys[i], &defaultPhrases));
	}
	assert(places.size()==placeLangKeys.size());
	for(uint32_t i=0; i<places.size(); i++){
		places[i].indication = phrases->getPhrase(placeLangKeys[i], &defaultPhrases);
	}
}

const std::vector<std::wstring>& FileSystemItemOrganizer::getItemFieldLabels() const{
	return fieldLabels;
}

bool FileSystemItemOrganizer::cd(const std::string& path){
	bool succ = fsys->changeWorkingDirectoryTo(path.c_str());
	if(succ){updateContent();}
	return succ;
}

bool FileSystemItemOrganizer::mkdir(const std::string& path){
	#ifdef _WIN32
	#error TODO Implement
	#else
	return ::mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP)==0;
	#endif
}

const std::string& FileSystemItemOrganizer::pwd() const{
	return wd;
}

std::vector<IItemOrganizer::Item*> FileSystemItemOrganizer::ls(uint32_t fieldIndexForSorting){
	std::vector<IItemOrganizer::Item*> res;
	res.reserve(content.size());
	for(uint32_t i=0; i<content.size(); i++){res.push_back(&(content[i]));}
	if(fieldIndexForSorting==1){//1==size
		std::function<bool(IItemOrganizer::Item* f1, IItemOrganizer::Item* f2)> isNameLess = createFieldLessComparator(0);//0==name
		std::sort(res.begin(), res.end(), [this,&isNameLess](IItemOrganizer::Item* f1, IItemOrganizer::Item* f2){
			if(f1->isDirectory!=f2->isDirectory){return f1->isDirectory;}//directories first
			if(fileSizes[f1->additionalID]==fileSizes[f2->additionalID]){//if same filesize (e.g. two directories) compare the name
				return isNameLess(f1, f2);
			}
			return fileSizes[f1->additionalID] < fileSizes[f2->additionalID];
		});
	}else{//other fields
		sortItemsByField(res, fieldIndexForSorting);
	}
	return res;
}
