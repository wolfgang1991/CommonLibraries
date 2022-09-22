#include "IAggregatableGUIElement.h"

#include <timing.h>

#include <IGUISkin.h>
#include <IGUIEnvironment.h>

#include <cassert>
#include <iostream>

using namespace irr;
using namespace gui;
using namespace core;

IAggregatableSkinExtension::IAggregatableSkinExtension(IExtendableSkin* skin, bool highlightIfActive, bool highlightIfPressed){
	this->skin = skin;
	this->highlightIfActive = highlightIfActive;
	this->highlightIfPressed = highlightIfPressed;
}

const std::string aggregatableSkinExtensionName = "AggregatableSkin";

const std::string& IAggregatableSkinExtension::getName(){
	return aggregatableSkinExtensionName;
}

void IAggregatableSkinExtension::drawHighlight(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& highlightRect, const irr::core::rect<irr::s32>* clip){
	if(highlightIfActive){
		skin->draw2DRectangle(ele, skin->getColor(EGDC_HIGH_LIGHT), highlightRect, clip);
	}
}

void IAggregatableSkinExtension::drawPressedHighlight(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& highlightRect, const irr::core::rect<irr::s32>* clip){
	if(highlightIfPressed){
		skin->draw2DRectangle(ele, skin->getColor(EGDC_ICON_HIGH_LIGHT), highlightRect, clip);
	}
}

IAggregatableGUIElement::IAggregatableGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, irr::f32 recommendedActiveSpace, irr::f32 activeAspectRatio, bool maintainAspectRatio, bool isActivateAble, irr::s32 id, void* data, IGUIElement* parent , const irr::core::rect<irr::s32>& rectangle):
	irr::gui::IGUIElement(irr::gui::EGUIET_ELEMENT, environment, parent, id, rectangle),
	recommendedSpace(recommendedSpace),
	aspectRatio(aspectRatio),
	recommendedActiveSpace(recommendedActiveSpace),
	activeAspectRatio(activeAspectRatio),
	maintainAspectRatio(maintainAspectRatio),
	isActivateAble(isActivateAble),
	active(false),
	activationLock(false),
	pressedInside(false),
	data(data),
	onClick([](IAggregatableGUIElement* ele){}),
	onLongTap([](IAggregatableGUIElement* ele){}),
	longTapTime(1000.0),//1000s to "deactivate"
	rectWhenPressed(0,0,0,0),
	timeWhenPressed(0.0)
{
	setName("IAggregatableGUIElement");
	if(parent==NULL){environment->getRootGUIElement()->addChild(this);}
	assert(getReferenceCount()==2);
	drop();//correct reference count since the reference is hold by the parent
}

void IAggregatableGUIElement::setLongTapCallback(const std::function<void(IAggregatableGUIElement* ele)>& callback, double longTapTime){
	onLongTap = callback;
	this->longTapTime = longTapTime;
}

void IAggregatableGUIElement::setOnClickCallback(const std::function<void(IAggregatableGUIElement* ele)>& callback){
	onClick = callback;
}

void IAggregatableGUIElement::setActivableFlag(bool isActivateAble){
	this->isActivateAble = isActivateAble;
}
	
bool IAggregatableGUIElement::getActivableFlag() const{
	return isActivateAble;
}

irr::f32 IAggregatableGUIElement::getOtherSpaceForAspectRatio(bool parentHorizontal, irr::f32 parentSpace, irr::f32 fallback){
	if(maintainAspectRatio){
		f32 thisSpace = getThisSpace(parentSpace);
		f32 ar = active?aspectRatio:activeAspectRatio;
		return parentHorizontal?(thisSpace/ar):(thisSpace*ar);
	}
	return fallback;
}

irr::f32 IAggregatableGUIElement::getThisSpace(irr::f32 parentSpace){
	return getRecommendedSpace()*parentSpace;
}

void* IAggregatableGUIElement::getData(){
	return data;
}
	
void IAggregatableGUIElement::setData(void* data){
	this->data = data;
}

irr::f32 IAggregatableGUIElement::getAspectRatio(){
	return active?activeAspectRatio:aspectRatio;
}

bool IAggregatableGUIElement::mustMaintainAspectRatio(){
	return maintainAspectRatio;
}

irr::f32 IAggregatableGUIElement::getRecommendedSpace(){
	return active?recommendedActiveSpace:recommendedSpace;
}

bool IAggregatableGUIElement::isActive(){
	return active;
}

void IAggregatableGUIElement::draw(){
	if(isVisible()){
		IGUISkin* skin = Environment->getSkin();
		if(isExtendableSkin(skin)){
			IAggregatableSkinExtension* extension = (IAggregatableSkinExtension*)((IExtendableSkin*)skin)->getExtension(aggregatableSkinExtensionName, getID());
			if(extension!=NULL){
				if(active){
					extension->drawHighlight(this, AbsoluteRect, &AbsoluteClippingRect);
				}else if(pressedInside){
					extension->drawPressedHighlight(this, AbsoluteRect, &AbsoluteClippingRect);
				}
			}
		}
		irr::gui::IGUIElement::draw();
		if(pressedInside && getSecs()-timeWhenPressed>longTapTime){
			pressedInside = false;//no click event when longtap event
			onLongTap(this);
		}
	}
}

bool IAggregatableGUIElement::OnEvent(const irr::SEvent& event){
	grab();//to avoid delete in events below
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		if(m.Event==EMIE_LMOUSE_LEFT_UP && pressedInside){
			pressedInside = false;
			if(isActivateAble && !activationLock){
				setActive(!isActive());
			}
			vector2d<s32> mPos(m.X, m.Y);
			if(AbsoluteRect.isPointInside(mPos) && rectWhenPressed.isPointInside(mPos)){
				onClick(this);
			}
		}else if(m.Event==EMIE_MOUSE_MOVED){
			vector2d<s32> mPos(m.X, m.Y);
			pressedInside = pressedInside && AbsoluteRect.isPointInside(mPos) && rectWhenPressed.isPointInside(mPos);
		}else if(m.Event==EMIE_LMOUSE_PRESSED_DOWN){// && getAbsolutePosition().isPointInside(vector2d<s32>(m.X,m.Y))){
			pressedInside = true;
			rectWhenPressed = AbsoluteRect;
			timeWhenPressed = getSecs();
		}
	}
	bool res = irr::gui::IGUIElement::OnEvent(event);
	drop();//see grab above
	return res;
}

void IAggregatableGUIElement::setActivationLock(bool on){
	activationLock = on;
}

void IAggregatableGUIElement::setActive(bool active, bool emitEventOnChange){
	bool prevActive = this->active;
	this->active = active;
	if(active!=prevActive && emitEventOnChange && Parent!=NULL){
		irr::SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent = SEvent::SGUIEvent{this, Parent, EGET_COUNT};
		Parent->OnEvent(event);
	}
}

void IAggregatableGUIElement::setRecommendedSpace(irr::f32 space){
	recommendedSpace = space;
}
	
void IAggregatableGUIElement::setRecommendedActiveSpace(irr::f32 space){
	recommendedActiveSpace = space;
}
	
