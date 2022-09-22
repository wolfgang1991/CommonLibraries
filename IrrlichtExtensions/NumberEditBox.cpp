#include <NumberEditBox.h>
#include <StringHelpers.h>

#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>
#include <IEventReceiver.h>

#include <iostream>

using namespace irr;
using namespace core;
using namespace gui;

NumberEditBox::NumberEditBox(irr::gui::IGUIEnvironment* env, irr::gui::IGUIElement* parent, irr::s32 id, const irr::core::rect<irr::s32> &rectangle, irr::s32 minValue, irr::s32 maxValue, irr::s32 largeStep, irr::s32 normalStep, irr::s32 defaultValue, irr::s32 buttonWidth):
	irr::gui::IGUIElement(EGUIET_ELEMENT, env, parent, id, rectangle),
	env(env),
	minValue(minValue),
	maxValue(maxValue),
	largeStep(largeStep),
	normalStep(normalStep),
	value(defaultValue){
	etext = env->addStaticText(stringw(defaultValue).c_str(), rect<s32>(2*buttonWidth, 0, rectangle.getWidth()-2*buttonWidth, rectangle.getHeight()), false, true, this, -1, false);
	etext->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	ldbut = env->addButton(rect<s32>(0, 0, buttonWidth, rectangle.getHeight()), this, -1, L"--", NULL);
	dbut = env->addButton(rect<s32>(buttonWidth, 0, 2*buttonWidth, rectangle.getHeight()), this, -1, L"-", NULL);
	ibut = env->addButton(rect<s32>(rectangle.getWidth()-2*buttonWidth, 0, rectangle.getWidth()-buttonWidth, rectangle.getHeight()), this, -1, L"+", NULL);
	libut = env->addButton(rect<s32>(rectangle.getWidth()-buttonWidth, 0, rectangle.getWidth(), rectangle.getHeight()), this, -1, L"++", NULL);
	for(int i=0; i<5; i++){
		IGUIElement* ele = i==0?static_cast<IGUIElement*>(etext):static_cast<IGUIElement*>(i==1?ldbut:(i==2?dbut:(i==3?ibut:libut)));
		ele->setSubElement(true);
		ele->setTabStop(false);
		ele->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	}
	etext->grab();
	ldbut->grab();
	dbut->grab();
	ibut->grab();
	libut->grab();
}
	
NumberEditBox::~NumberEditBox(){
	etext->drop();
	ldbut->drop();
	dbut->drop();
	libut->drop();
	ibut->drop();
}

irr::s32 NumberEditBox::getValue() const{
	return value;
}

void NumberEditBox::setValue(irr::s32 value){
	this->value = value;
	etext->setText(convertToWString(value).c_str());
}

irr::gui::IGUIStaticText* NumberEditBox::getStaticText() const{
	return etext;
}

irr::gui::IGUIButton* NumberEditBox::getLargeDecrementButton() const{
	return ldbut;
}

irr::gui::IGUIButton* NumberEditBox::getDecrementButton() const{
	return dbut;
}

irr::gui::IGUIButton* NumberEditBox::getLargeIncrementButton() const{
	return libut;
}

irr::gui::IGUIButton* NumberEditBox::getIncrementButton() const{
	return ibut;
}

const wchar_t* NumberEditBox::getText() const{
	return etext->getText();
}

void NumberEditBox::setText(const wchar_t* text){
	etext->setText(text);
	value = convertWStringTo<irr::s32>(text);
}

bool NumberEditBox::OnEvent(const irr::SEvent& event){
	bool res = irr::gui::IGUIElement::OnEvent(event);
	if(!res){
		if(event.EventType==EET_GUI_EVENT){
			const SEvent::SGUIEvent& g = event.GUIEvent;
			if(g.EventType==EGET_BUTTON_CLICKED){
				irr::s32 oldValue = value;
				if(g.Caller==ldbut){
					value -= largeStep;
				}else if(g.Caller==dbut){
					value -= normalStep;
				}else if(g.Caller==libut){
					value += largeStep;
				}else if(g.Caller==ibut){
					value += normalStep;
				}else{
					return res;
				}
				if(value<minValue){
					value = minValue;
				}else if(value>maxValue){
					value = maxValue;
				}
				if(value!=oldValue){
					SEvent event;
					event.EventType = EET_GUI_EVENT;
					event.GUIEvent.Caller = this;
					event.GUIEvent.EventType = EGET_COUNT;
					if(Parent){Parent->OnEvent(event);}
				}
				etext->setText(stringw(value).c_str());
			}
		}
	}
	return res;
}

NumberEditBox* addNumberEditBox(irr::gui::IGUIEnvironment* env, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32> &rectangle, irr::s32 buttonWidth, irr::s32 minValue, irr::s32 maxValue, irr::s32 largeStep, irr::s32 normalStep, irr::s32 defaultValue, irr::s32 id){
	parent = parent?parent:(env->getRootGUIElement());
	NumberEditBox* box = new NumberEditBox(env, parent, id, rectangle, minValue, maxValue, largeStep, normalStep, defaultValue, buttonWidth);
	box->drop();
	return box;
}
