#include "StringHelpers.h"

#include <iomanip>
#include <vector>
#include <cmath>
#include <cstring>

template<class E,
 class T = std::char_traits<E>,
 class A = std::allocator<E> >
 
 class Widen : public std::unary_function<
     const std::string&, std::basic_string<E, T, A> >
 {
     std::locale loc_;
     const std::ctype<E>* pCType_;
 
     // No copy-constructor, no assignment operator...
     Widen(const Widen&);
     Widen& operator= (const Widen&);
 
 public:
     // Constructor...
     Widen(const std::locale& loc = std::locale()) : loc_(loc)
     {
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6.0...
         using namespace std;
         pCType_ = &_USE(loc, ctype<E> );
#else
         pCType_ = &std::use_facet<std::ctype<E> >(loc);
#endif
     }
 
     // Conversion...
     std::basic_string<E, T, A> operator() (const std::string& str) const
     {
         typename std::basic_string<E, T, A>::size_type srcLen = str.length();
         const char* pSrcBeg = str.c_str();
         std::vector<E> tmp(srcLen);
 
         pCType_->widen(pSrcBeg, pSrcBeg + srcLen, &tmp[0]);
         return std::basic_string<E, T, A>(&tmp[0], srcLen);
     }
 };

Widen<wchar_t> to_wstring;

std::wstring convertStringToWString(const std::string& str){
	return to_wstring(str);
}

std::string convertWStringToString(const std::wstring& str){
	return std::string(str.begin(), str.end());//does not recognize encoding correctly (for non ascii chars)
}

std::list<std::string> parseSeparatedString(const std::string& s, char separator){
	std::list<std::string> res;
	parseSeparatedString(res, s, separator);
	return res;
}

bool isAnInteger(const std::string& c){
	if(c.size()>=1){
		if((c[0]<'0' || c[0]>'9') && c[0]!='-'){return false;}
	}
	for(unsigned int i=1; i<c.size(); i++){
		if(c[i]<'0' || c[i]>'9'){return false;}//Fehlerzustand
	}
	return true;
}

std::string getHumanReadableSpace(uint64_t space){
	bool lessGB = space<100*1000*1000;
	bool lessMB = space<100*1000;
	return round(lessGB?(lessMB?double(space)/(1000.0):double(space)/(1000.0*1000.0)):(double(space)/(1000.0*1000.0*1000.0)),2).append(lessGB?(lessMB?"KB":"MB"):"GB");
}

//! returns -1 if not available
static int32_t findLastSlashPos(const std::string& str){
	for(int32_t i=str.size()-1; i>=0; i--){
		char c = str[i];
		if(c=='/' || c=='\\'){return i;}
	}
	return -1;
}

std::string stripDir(const std::string& str){
	int32_t sp = findLastSlashPos(str);
	return str.substr(sp+1, std::string::npos);
}

std::string stripFile(const std::string& path){
	int32_t sp = findLastSlashPos(path);
	if(sp>=0){
		return path.substr(0, sp+1);
	}else{
		return "";
	}
}

//! -1 if not available
static int findDotPos(const std::string& str){
	int dotPos = -1;
	for(int i = (int)str.size()-1; i>=0; i--){
		if(str[i]=='.'){
			dotPos = i;
			break;
		}
	}
	return dotPos;
}

std::string stripExt(const std::string& str){
	int dotPos = findDotPos(str);
	return dotPos>=0?str.substr(0,dotPos):str;
}

std::string getExt(const std::string& str){
	int dotPos = findDotPos(str);
	return dotPos>=0?str.substr(dotPos+1,str.size()):"";
}

std::string getHumanReadableTime(uint64_t time, bool showDays){
	uint64_t secs = time%60;
	uint64_t mins = (time%3600)/60;
	uint64_t hours = (time%86400)/3600;
	uint64_t days = time/86400;
	std::stringstream ss;
	if(days>0 && showDays){
		ss << days << "d ";
	}else{
		hours += days*24;
	}
	ss << std::setw(2) << std::setfill('0') << hours << ':' << std::setw(2) << std::setfill('0') << mins << ':' << std::setw(2) << std::setfill('0') << secs;
	return ss.str();
}

bool isGlobalPath(std::string path){
	bool isGlobal = false;
	if(path.size()>0){
		if(path[0]=='/'){
			isGlobal = true;//linux
		}else if(path.size()>2){
			isGlobal = path[1]==':' && (path[2]=='\\' || path[2]=='/');//windows
		}
	}
	return isGlobal;
}

std::string getAppHomePathFromArgV0(const char* argv0){
	std::string apphome("./");
	std::string p(argv0);
	int lastSlash = -1;
	for(int i=p.size()-1; i>=0; i--){
		char c = p[i];
		if(c=='/' || c=='\\'){
			lastSlash = i;
			break;
		}
	}
	if(lastSlash>=0){
		if(isGlobalPath(p)){
			apphome = p.substr(0,lastSlash+1);
		}else if(isPrefixEqual(p, "./")){
			apphome.append(p.substr(2,lastSlash+1-2));
		}else{
			apphome.append(p.substr(0,lastSlash+1));
		}
	}
	return apphome;
}

bool getCommandLineArgument(std::string& value, int argc, char* argv[], const std::string& argument, const std::string& defaultValue, char argPrefix){
	for(int i=0; i<argc; i++){
		std::string s(argv[i]);
		if(isPrefixEqual(s, argument)){
			if(s==argument){//-arg value or -arg -nextarg
				if(i<argc-1 && argv[i+1][0]!=argPrefix){//-arg value
					value = argv[i+1];
				}else{//-arg -nextarg
					value = "";
				}
				return true;
			}else{// -argvalue
				value = s.substr(argument.size(), std::string::npos);
				return true;
			}
		}
	}
	value = defaultValue;
	return false;
}

bool hasCommandLineArgument(int argc, char* argv[], const char* argument){
	for(int i=0; i<argc; i++){
		if(strcmp(argument, argv[i])==0){return true;}
	}
	return false;
}
