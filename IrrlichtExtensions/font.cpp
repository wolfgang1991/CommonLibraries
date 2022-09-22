#include "font.h"

#include <IGUIFont.h>
#include <IGUIElement.h>

#include <sstream>

using namespace irr;
using namespace gui;
using namespace core;

std::wstring makeWordWrappedText(const std::wstring& text, irr::u32 maxWidth, irr::gui::IGUIFont* font, bool withHyphen, bool removeTrailingNewLines){
	int lastWhitespace = -1;
	uint32_t start = 0;
	std::wstringstream ss;
	for(uint32_t i=0; i<text.size(); i++){
		wchar_t c = text[i];
		if(c==L'\n'){//important to get O(m*n) (m max. line length (which is constant in case of reasonable maxWidth), n #chars), otherwise it would be O(n^2)
			ss << text.substr(start, i-start+1);//with current char
			start = i+1;
			lastWhitespace = -1;
		}else{
			if(c==L' ' || c=='\t'){
				lastWhitespace = (int)i;
			}
			std::wstring sub = text.substr(start, i-start+1);
			dimension2d<u32> dim = font->getDimension(sub.c_str());//O(sub.size())
			if(dim.Width>maxWidth){
				if(lastWhitespace!=-1){//regular newline at last whitespace
					ss << text.substr(start, lastWhitespace-start) << L"\n";//beginning without whitespace
					start = lastWhitespace+1;//part after whitespace
					lastWhitespace = -1;
				}else if(i>start+1){//force newline without whitespace
					ss << text.substr(start, i-start-1) << (withHyphen?L"-\n":L"\n");//without current and previous char but with -
					start = i-1;
					lastWhitespace = -1;
				}else{//no space left but still force newline
					ss << text.substr(start, i-start+1);//with current char
					start = i+1;
					lastWhitespace = -1;
				}
			}
		}
	}
	if(start<text.size()){
		ss << text.substr(start, std::string::npos);
	}
	//remove newlines created at end because of many unnecessary whitespaces
	if(removeTrailingNewLines){
		std::wstring result = ss.str();
		while(result.size()>0){
			if(result[result.size()-1]==L'\n'){
				result = result.substr(0, result.size()-1);
			}else{
				break;
			}
		}
		return result;
	}else{
		return ss.str();
	}
}

void breakGUIElementText(irr::gui::IGUIElement* ele, irr::gui::IGUIFont* font, irr::f32 horizontalPaddingPart, bool withHyphen, bool removeTrailingNewLines){
	u32 eleWidth = ele->getRelativePosition().getWidth();
	ele->setText(makeWordWrappedText(ele->getText(), eleWidth-2.f*horizontalPaddingPart*eleWidth, font, withHyphen, removeTrailingNewLines).c_str());
}
