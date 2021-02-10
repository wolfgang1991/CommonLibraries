#include <CMBox.h>
#include <font.h>
#include <utilities.h>
#include <mathUtils.h>

#include <irrlicht.h>

#include <string>
#include <map>

using namespace std;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

void bringToFrontRecursive(irr::gui::IGUIElement* ele){
	while(ele->getParent()){
		ele->getParent()->bringToFront(ele);
		ele = ele->getParent();
	}
}

CMBox::CMBox(irr::IrrlichtDevice* device, std::wstring text, irr::f32 maxW, irr::f32 maxH, const wchar_t* positive, const wchar_t* negative, irr::s32 posId, irr::s32 negId, bool modal, irr::gui::EGUI_ALIGNMENT horizontal, irr::gui::EGUI_ALIGNMENT vertical):
	IGUIElement(irr::gui::EGUIET_ELEMENT, device->getGUIEnvironment(), NULL, -1, modal?rect<s32>(0,0,device->getVideoDriver()->getScreenSize().Width,device->getVideoDriver()->getScreenSize().Height):rect<s32>(0,0,0,0)){
	env = device->getGUIEnvironment();
	env->getRootGUIElement()->addChild(this);
	driver = device->getVideoDriver();
	w = driver->getScreenSize().Width;
	h = driver->getScreenSize().Height;
	const s32 buttonHeight = (s32)(0.0625*0.79057*sqrt(w*h));
	IGUIFont* font = env->getSkin()->getFont();
	s32 textW = 0, textH = 0;
	s32 skinTextPaddingX = env->getSkin()->getSize(EGDS_TEXT_DISTANCE_X);
	s32 padding = Max(skinTextPaddingX, (s32)round_(0.01*sqrt(w*h)));
	s32 maxTW = (s32)(maxW*w+0.5f)-2*padding;
	text = makeWordWrappedText(text, maxTW, font);
	dimension2d<u32> dim = font->getDimension(text.c_str());
	textW = Min(maxTW, (s32)dim.Width);
	s32 buttonWidth = (textW/2-5>0.3f*w?(0.3f*w):(textW/2-5));
	std::wstring finalPositive = positive?makeWordWrappedText(positive, 9*buttonWidth/10, font):std::wstring(L"");
	std::wstring finalNegative = negative?makeWordWrappedText(negative, 9*buttonWidth/10, font):std::wstring(L"");
	const s32 finalButtonHeight = Max(buttonHeight, 10*(s32)Max(font->getDimension(finalPositive.c_str()).Height, font->getDimension(finalNegative.c_str()).Height)/9);
	s32 maxTH = (s32)(maxH*h+0.5f)-3*padding-finalButtonHeight;
	textH = Min(maxTH, (s32)dim.Height);
	win = env->addWindow(rect<s32>(w/2-(textW+2*padding)/2, h/2-(textH+3*padding+finalButtonHeight)/2, w/2+(textW+2*padding)/2, h/2+(textH+3*padding+finalButtonHeight)/2), modal, L"", this, -1);
	win->getCloseButton()->setVisible(false); win->setDrawTitlebar(false);
	win->setNotClipped(true);
	stext = env->addStaticText(text.c_str(), rect<s32>(padding-skinTextPaddingX, padding, padding+textW+skinTextPaddingX, padding+textH), false, true, win, -1, false);//, true, true, win, -1, false);//
	stext->setTextAlignment(horizontal, vertical);
	stext->setWordWrap(false);
	pos = neg = NULL;
	if(positive && negative){
		neg = env->addButton(rect<s32>(10, padding+textH+padding, 10+buttonWidth, padding+textH+padding+finalButtonHeight), win, negId, finalNegative.c_str(), NULL);
		pos = env->addButton(rect<s32>(10+textW-buttonWidth, padding+textH+padding, 10+textW, padding+textH+padding+finalButtonHeight), win, posId, finalPositive.c_str(), NULL);
	}else if(positive){
		pos = env->addButton(rect<s32>(10+textW/2-buttonWidth/2, padding+textH+padding, 10+textW/2+buttonWidth/2, padding+textH+padding+finalButtonHeight), win, posId, finalPositive.c_str(), NULL);
	}
	env->setFocus(pos?(IGUIElement*)pos:(IGUIElement*)win);
	bringToFrontRecursive(win);
	drop();//drop since the valid reference is hold by the parent (root element)
}

CMBox::~CMBox(){
}

void CMBox::OnNegative(){
	SEvent toPost;
	toPost.EventType = EET_GUI_EVENT;
	toPost.GUIEvent.Caller = this;
	toPost.GUIEvent.Element = neg;
	toPost.GUIEvent.EventType = EGET_MESSAGEBOX_NO;
	Parent->OnEvent(toPost);
	setVisible(false);
	remove();
}
	
void CMBox::OnPositive(){
	SEvent toPost;
	toPost.EventType = EET_GUI_EVENT;
	toPost.GUIEvent.Caller = this;
	toPost.GUIEvent.Element = pos;
	toPost.GUIEvent.EventType = EGET_MESSAGEBOX_YES;
	Parent->OnEvent(toPost);
	setVisible(false);
	remove();
}

bool CMBox::OnEvent(const irr::SEvent& event){
	bool res = IGUIElement::OnEvent(event);
	if(!res){//check because CMBox may have been deleted otherwise
		if(event.EventType==EET_GUI_EVENT){
			if(event.GUIEvent.EventType==EGET_BUTTON_CLICKED){
				IGUIElement* source = event.GUIEvent.Caller;
				if(source==pos){
					OnPositive();
					return true;
				}else if(source==neg){
					OnNegative();
					return true;
				}
			}
		}else if(event.EventType==EET_KEY_INPUT_EVENT){
			if(event.KeyInput.Key==KEY_BROWSER_BACK && !event.KeyInput.PressedDown){
				if(neg){
					OnNegative();
				}else if(pos){
					OnPositive();
				}else{
					setVisible(false);
					remove();
				}
				return true;
			}
		}
	}
	return res;
}

void CMBox::OnPostRender(u32 timeMs){
	IGUIElement::OnPostRender(timeMs);
	win->setRelativePosition(limitRect(win->getRelativePosition(),env->getRootGUIElement()->getRelativePosition()));
}
