#include "StringHelpers.h"

#include <iomanip>
#include <vector>
#include <cmath>
#include <cstring>


std::list<std::string> parseSeparatedString(const std::string& s, char separator, bool noEmptyToken){
	std::list<std::string> res;
	parseSeparatedString(res, s, std::string(1, separator), noEmptyToken);
	return res;
}

std::string stripSpaces(const std::string s){
	int start = 0;
	for(; start < (int)(s.size()); start++){
		if(s[start] != ' '){
			break;
		}
	}
	int end = (int)(s.size())-1;
	for(; end >= 0; end--){
		if(s[end] != ' '){
			break;
		}
	}
	if(start<=end){
		return s.substr(start, end-start+1);
	}else{
		return "";
	}
}

bool isADouble(const std::string& c){
	bool dotRead = false;
	if(c.size()>=1){
		if(c[0]=='.'){
			dotRead = true;
		}else if((c[0]<'0' || c[0]>'9') && c[0]!='-'){
			return false;
		}
	}
	for(unsigned int i=1; i<c.size(); i++){
		if(c[i]=='.'){
			if(dotRead){return false;}
			dotRead = true;
		}else if(c[i]<'0' || c[i]>'9'){
			return false;
		}
	}
	return true;
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

std::string convertStringToUpper(const std::string& s){
	std::string ret(s.size(), char());
	for(unsigned int i = 0; i < s.size(); ++i){
		ret[i] = (s[i] <= 'z' && s[i] >= 'a') ? s[i]-'a'+'A' : s[i];
	}
	return ret;
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

bool isPositiveInteger(const std::string& s){
	for(unsigned int i=0; i<s.size(); i++){
		char c = s[i];
		if(c<'0' || c>'9'){return false;}
	}
	return true;
}

std::string cleanPath(const std::string& path){
	std::stringstream ss;
	bool slashRead = false;
	for(uint32_t i=0; i<path.size(); i++){
		char c = path[i];
		if(c=='/'){
			if(!slashRead){
				slashRead = true;
				ss << c;
			}
		}else{
			slashRead = false;
			ss << c;
		}
	}
	return ss.str();
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
