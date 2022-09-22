#include "AMLBox.h"
#include "ICommonAppContext.h"
#include "AMLGUIElement.h"
#include "CMBox.h"
#include "utilities.h"

#include <IVideoDriver.h>
#include <IrrlichtDevice.h>
#include <IGUIWindow.h>
#include <IGUIButton.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

AMLBox::AMLBox(ICommonAppContext* c, irr::s32 aggregationStyleID, irr::s32 invisibleAggregationStyleId, irr::s32 scrollbarStyleId, const AMLGUIElement::NavButtons* navButtons, const std::string& language, irr::f32 maxW, irr::f32 maxH, const wchar_t* buttonText, irr::s32 buttonId, bool modal, irr::s32 navButtonId, irr::video::SColor* overrideTextColor):
	IGUIElement(irr::gui::EGUIET_ELEMENT, c->getIrrlichtDevice()->getGUIEnvironment(), NULL, -1, modal?rect<s32>(0,0,c->getIrrlichtDevice()->getVideoDriver()->getScreenSize().Width,c->getIrrlichtDevice()->getVideoDriver()->getScreenSize().Height):rect<s32>(0,0,0,0)){
	env = c->getIrrlichtDevice()->getGUIEnvironment();
	env->getRootGUIElement()->addChild(this);
	setCloseOnScreenResize(true);
	mustDelete = false;
	driver = c->getIrrlichtDevice()->getVideoDriver();
	screenSize = driver->getScreenSize();
	w = screenSize.Width;
	h = screenSize.Height;
	const s32 buttonHeight = (s32)(0.0625*0.79057*sqrt(w*h));
	int ww = maxW*w, wh = maxH*h;
	win = env->addWindow(rect<s32>(w/2-ww/2, h/2-wh/2, w/2+ww/2, h/2+wh/2), modal, L"", this, -1);
	win->getCloseButton()->setVisible(false); win->setDrawTitlebar(false);
	win->setNotClipped(true);
	if(ww>=w && wh>=h){win->setDraggable(false);}
	ok = env->addButton(rect<s32>(ww/4, wh-buttonHeight-10, 3*ww/4, wh-10), win, buttonId, buttonText, NULL);
	env->setFocus(ok);
	aml = new AMLGUIElement(c, 1.f, 1.f, false, aggregationStyleID, invisibleAggregationStyleId, scrollbarStyleId, ".", language, navButtons, win, irr::core::rect<irr::s32>(10,10,ww-10,wh-buttonHeight-20), navButtonId, overrideTextColor);
	win->addChild(aml);
	IGUIElement* ele = win;
	while(ele->getParent()){
		ele->getParent()->bringToFront(ele);
		ele = ele->getParent();
	}
	drop();
}

irr::gui::IGUIButton* AMLBox::getButton() const{
	return ok;
}

void AMLBox::setCloseOnScreenResize(bool closeOnScreenResize){
	this->closeOnScreenResize = closeOnScreenResize;
}

bool AMLBox::setCode(const std::string& utf8Code, bool printParsingErrors){
	return aml->setCode(utf8Code, printParsingErrors);
}

void AMLBox::gotoFile(const std::string& path){
	return aml->gotoFile(path);
}

AMLBox::~AMLBox(){
	win->remove();
}

bool AMLBox::OnEvent(const irr::SEvent& event){
	bool res = IGUIElement::OnEvent(event);
	if(!res){//check because Box may have been deleted otherwise
		if(event.EventType==EET_GUI_EVENT){
			if(event.GUIEvent.EventType==EGET_BUTTON_CLICKED){
				if(event.GUIEvent.Caller==ok){
					remove();
					return true;
				}
			}
		}else if(event.EventType==EET_KEY_INPUT_EVENT){
			if(event.KeyInput.Key==KEY_BROWSER_BACK && !event.KeyInput.PressedDown){
				aml->goBack();
				return true;
			}
		}
	}
	if(mustDelete){remove();}
	return res;
}

void AMLBox::OnPostRender(irr::u32 timeMs){
	IGUIElement::OnPostRender(timeMs);
	win->setRelativePosition(limitRect(win->getRelativePosition(),env->getRootGUIElement()->getRelativePosition()));
	if(closeOnScreenResize && driver->getScreenSize()!=screenSize){
		mustDelete = true;
	}
}
