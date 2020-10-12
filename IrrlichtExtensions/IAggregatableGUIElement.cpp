#include "IAggregatableGUIElement.h"

#include <IGUISkin.h>
#include <IGUIEnvironment.h>

#include <cassert>
#include <iostream>

using namespace irr;
using namespace gui;
using namespace core;

IAggregatableSkinExtension::IAggregatableSkinExtension(IExtendableSkin* skin, bool highlightIfActive){
	this->skin = skin;
	this->highlightIfActive = highlightIfActive;
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
	data(data)
{
	setName("IAggregatableGUIElement");
	if(parent==NULL){environment->getRootGUIElement()->addChild(this);}
	assert(getReferenceCount()==2);
	drop();//correct reference count since the reference is hold by the parent
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
		if(active && isExtendableSkin(skin)){
			((IAggregatableSkinExtension*)((IExtendableSkin*)skin)->getExtension(aggregatableSkinExtensionName, getID()))->drawHighlight(this, AbsoluteRect, &AbsoluteClippingRect);
		}
		irr::gui::IGUIElement::draw();
	}
}

bool IAggregatableGUIElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		if(m.Event==EMIE_LMOUSE_LEFT_UP && isActivateAble && !activationLock && pressedInside){
			pressedInside = false;
			setActive(!isActive());
		}else if(m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			pressedInside = true;
		}
	}
	return irr::gui::IGUIElement::OnEvent(event);
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
	
