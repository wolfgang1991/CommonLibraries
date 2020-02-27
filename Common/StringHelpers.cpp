#include "StringHelpers.h"

#include <iomanip>
#include <vector>
#include <cmath>

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

std::list<std::string> parseSeparatedString(const std::string& s, char seperator){
	std::list<std::string> res;
	std::stringstream ss;
	for(unsigned int i=0; i<s.size(); i++){
		char c = s[i];
		if(c==seperator){
			res.push_back(ss.str());
			ss.str("");
		}else{
			ss << c;
		}
	}
	if(ss.str().size()!=0){res.push_back(ss.str());}
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

std::string round(double val, int acc){
	std::stringstream res;
	res.precision(acc);
	res.setf(std::ios::fixed, std::ios::floatfield);
	res << val;
	return res.str();
}

std::string getHumanReadableSpace(uint64_t space){
	bool lessGB = space<100*1000*1000;
	bool lessMB = space<100*1000;
	return round(lessGB?(lessMB?double(space)/(1000.0):double(space)/(1000.0*1000.0)):(double(space)/(1000.0*1000.0*1000.0)),2).append(lessGB?(lessMB?"KB":"MB"):"GB");
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
