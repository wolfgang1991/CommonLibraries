#include "FileSystemItemOrganizer.h"
#include "ConstantLanguagePhrases.h"

#include <utf8.h>
#include <StringHelpers.h>
#include <BitFunctions.h>

#include <IFileSystem.h>
#include <IrrlichtDevice.h>

#include <iomanip>
#include <sstream>
#include <ctime>
#include <iostream>

#include <sys/stat.h>

#ifdef _WIN32
#include <fileapi.h>
#include <shlobj.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include <android/log.h>

static std::wstring Java_To_WStr(JNIEnv *env, jstring string){
    std::wstring value;
    const jchar *raw = env->GetStringChars(string, 0);
    jsize len = env->GetStringLength(string);
    value.assign(raw, raw + len);
    env->ReleaseStringChars(string, raw);
    return value;
}
#endif

using namespace irr;
using namespace core;
using namespace io;


static const ConstantLanguagePhrases defaultPhrases({
	{L"File::NAME",L"Name"},
	{L"File::SIZE",L"Size"},
	{L"File::MODIFICATION",L"Modified"},
	{L"Place::HOME",L"Home"},
	{L"Place::ROOT",L"/"},
	{L"Place::DOCUMENTS",L"Documents"},
	{L"Place::DOWNLOADS",L"Downloads"}
});

FileSystemItemOrganizer::FileSystemItemOrganizer(irr::IrrlichtDevice* device):fsys(device->getFileSystem()),fieldLangKeys{L"File::NAME", L"File::SIZE", L"File::MODIFICATION"}{
	#ifdef _WIN32
	//Home
	WCHAR path[MAX_PATH];
	if(SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path))) {
		placeLangKeys.push_back(L"Place::HOME");
		places.push_back(IItemOrganizer::Place{L"", convertWStringToUtf8String(path)});
	}
	//Drives
	DWORD driveMask = GetLogicalDrives();
	std::string drivePath = "C:";
	uint32_t bitCount = sizeof(driveMask)*CHAR_BIT;
	for(uint32_t i=0; i<bitCount; i++){
		bool hasDrive = getBit(driveMask, i);
		if(hasDrive){
			char driveLetter = 'A'+(char)i;
			drivePath[0] = driveLetter;
			std::wstring wDrivePath = convertUtf8ToWString(drivePath);
			placeLangKeys.push_back(wDrivePath);
			places.push_back(IItemOrganizer::Place{wDrivePath, drivePath});
		}
	}
	#elif defined(__ANDROID__)
//	placeLangKeys.push_back(L"Place::ROOT");
//	places.push_back(IItemOrganizer::Place{L"", "/"});
	{
		JNIEnv* JNIEnvAttachedToVM;
		JavaVMAttachArgs attachArgs;
		attachArgs.version = JNI_VERSION_1_6;
		attachArgs.name = 0;
		attachArgs.group = NULL;
		// Get the interface to the native Android activity.
		android_app* app = (android_app*)(device->getCreationParams().PrivateData);
		// Not a big problem calling it each time - it's a no-op when the thread already is attached.
		jint result = app->activity->vm->AttachCurrentThread(&JNIEnvAttachedToVM, &attachArgs);
		if(result == JNI_ERR){
			 __android_log_print(ANDROID_LOG_ERROR, "native", "AttachCurrentThread for the JNI environment failed.");
			JNIEnvAttachedToVM = 0;
		}else{
			jclass fileClass = JNIEnvAttachedToVM->FindClass("java/io/File");
			if(fileClass==NULL){
				__android_log_print(ANDROID_LOG_ERROR, "native", "java/io/File not found");
			}else{
				jmethodID getAbsolutePath = JNIEnvAttachedToVM->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
				if(getAbsolutePath==NULL){
					__android_log_print(ANDROID_LOG_ERROR, "native", "getAbsolutePath not found");
				}else{
					jclass environmentClass = JNIEnvAttachedToVM->FindClass("android/os/Environment");
					if(environmentClass==NULL){
						 __android_log_print(ANDROID_LOG_ERROR, "native", "android/os/Environment not found");
					}else{
						jmethodID getExternalStoragePublicDirectory = JNIEnvAttachedToVM->GetStaticMethodID(environmentClass, "getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;");
						if(getExternalStoragePublicDirectory==NULL){
							 __android_log_print(ANDROID_LOG_ERROR, "native", "getExternalStoragePublicDirectory method not found");
						}else{
							jfieldID DIRECTORY_DOCUMENTS = JNIEnvAttachedToVM->GetStaticFieldID(environmentClass, "DIRECTORY_DOCUMENTS", "Ljava/lang/String;");
							if(DIRECTORY_DOCUMENTS==NULL){
								__android_log_print(ANDROID_LOG_ERROR, "native", "DIRECTORY_DOCUMENTS static field not found");
							}else{
								jobject DIRECTORY_DOCUMENTS_STRING = JNIEnvAttachedToVM->GetStaticObjectField(environmentClass, DIRECTORY_DOCUMENTS);
								if(DIRECTORY_DOCUMENTS_STRING==NULL){
									__android_log_print(ANDROID_LOG_ERROR, "native", "Retrieving DIRECTORY_DOCUMENTS failed");
								}else{
									jfieldID DIRECTORY_DOWNLOADS = JNIEnvAttachedToVM->GetStaticFieldID(environmentClass, "DIRECTORY_DOWNLOADS", "Ljava/lang/String;");
									if(DIRECTORY_DOWNLOADS==NULL){
										__android_log_print(ANDROID_LOG_ERROR, "native", "DIRECTORY_DOWNLOADS static field not found");
									}else{
										jobject DIRECTORY_DOWNLOADS_STRING = JNIEnvAttachedToVM->GetStaticObjectField(environmentClass, DIRECTORY_DOWNLOADS);
										if(DIRECTORY_DOWNLOADS_STRING==NULL){
											__android_log_print(ANDROID_LOG_ERROR, "native", "Retrieving DIRECTORY_DOWNLOADS failed");
										}else{
											jobject dir = JNIEnvAttachedToVM->CallStaticObjectMethod(environmentClass, getExternalStoragePublicDirectory, DIRECTORY_DOCUMENTS_STRING);
											jstring absPath = (jstring)(JNIEnvAttachedToVM->CallObjectMethod(dir, getAbsolutePath));
											std::wstring absPathW = Java_To_WStr(JNIEnvAttachedToVM, absPath);
											placeLangKeys.push_back(L"Place::DOCUMENTS");
											places.push_back(IItemOrganizer::Place{L"", convertWStringToUtf8String(absPathW)});
											dir = JNIEnvAttachedToVM->CallStaticObjectMethod(environmentClass, getExternalStoragePublicDirectory, DIRECTORY_DOWNLOADS_STRING);
											absPath = (jstring)(JNIEnvAttachedToVM->CallObjectMethod(dir, getAbsolutePath));
											absPathW = Java_To_WStr(JNIEnvAttachedToVM, absPath);
											placeLangKeys.push_back(L"Place::DOWNLOADS");
											places.push_back(IItemOrganizer::Place{L"", convertWStringToUtf8String(absPathW)});
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	#elif defined(__APPLE__)
	#error add places FileSystemItemOrganizer: app home dir, documents dir
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

bool FileSystemItemOrganizer::doesExist(const std::string& path) const{
	return fsys->existFile(path.c_str());
}

std::string FileSystemItemOrganizer::getAbsolutePath(const std::string& relativePath) const{
	std::stringstream ss;
	#if _WIN32
	ss << wd << "\\" << relativePath;
	#else
	ss << wd << "/" << relativePath; 
	#endif
	return ss.str();
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
				struct stat64 s;
				if(stat64(filename.c_str(), &s)==0){
					#ifdef _WIN32
					modificationTime = s.st_mtime;
					#else
					modificationTime = s.st_mtim.tv_sec;
					#endif
					fileSize = s.st_size;
				}else{
					std::cerr << "stat64 error (" << errno << "): " << strerror(errno) << std::endl;
				}
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
	#if _WIN32
	bool succ = false;//fsys->changeWorkingDirectoryTo(path.size()<=2(path+"\\").c_str():path.c_str());
	if(path.size()==2){
		if(path[1]==':'){
			succ = fsys->changeWorkingDirectoryTo((path+"\\").c_str());
		}
	}
	if(!succ){succ = fsys->changeWorkingDirectoryTo(path.c_str());}
	#else
	bool succ = fsys->changeWorkingDirectoryTo(path.c_str());
	#endif
	if(succ){updateContent();}
	return succ;
}

bool FileSystemItemOrganizer::mkdir(const std::string& path){
	#ifdef _WIN32
	return _wmkdir(convertUtf8ToWString(path).c_str())==0;
	#else
	return ::mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP)==0;
	#endif
}

const std::string& FileSystemItemOrganizer::pwd() const{
	return wd;
}

std::vector<IItemOrganizer::Item*> FileSystemItemOrganizer::ls(uint32_t fieldIndexForSorting, bool sortAscending){
	std::vector<IItemOrganizer::Item*> res;
	res.reserve(content.size());
	for(uint32_t i=0; i<content.size(); i++){res.push_back(&(content[i]));}
	if(fieldIndexForSorting==1){//1==size
		std::function<bool(IItemOrganizer::Item* f1, IItemOrganizer::Item* f2)> isNameLess = createFieldLessComparator(0, sortAscending);//0==name
		std::sort(res.begin(), res.end(), [this,&isNameLess,sortAscending](IItemOrganizer::Item* f1, IItemOrganizer::Item* f2){
			if(f1->isDirectory!=f2->isDirectory){return f1->isDirectory;}//directories first
			if(fileSizes[f1->additionalID]==fileSizes[f2->additionalID]){//if same filesize (e.g. two directories) compare the name
				return sortAscending?isNameLess(f1,f2):isNameLess(f2,f1);
			}
			return sortAscending?(fileSizes[f1->additionalID]<fileSizes[f2->additionalID]):(fileSizes[f1->additionalID]>fileSizes[f2->additionalID]);
		});
	}else{//other fields
		sortItemsByField(res, fieldIndexForSorting, sortAscending);
	}
	return res;
}

#if _WIN32
static const std::vector<char> delimeter{'\\', '/'};
#else
static const std::vector<char> delimeter{'/'};
#endif

const std::vector<char>& FileSystemItemOrganizer::getPathDelimeter() const{
	return delimeter;
}
