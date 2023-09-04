#include "BeautifulGUIButton.h"

#include <IGUISkin.h>
#include <IEventReceiver.h>

using namespace irr;
using namespace gui;
using namespace core;

BeautifulGUIButton::BeautifulGUIButton(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, bool isHorizontal, bool isScrollable, const std::initializer_list<IAggregatableGUIElement*>& subElements, const std::initializer_list<IAggregatableGUIElement*>& activatedSubElements, irr::s32 id, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
		AggregateGUIElement(environment, recommendedSpace, aspectRatio, recommendedSpace, aspectRatio, maintainAspectRatio, isHorizontal, isScrollable, subElements, activatedSubElements, false, id, data, parent, rectangle){
	pressed = false;
	buttonEvent.EventType = EET_GUI_EVENT;
	buttonEvent.GUIEvent.Caller = this;
	buttonEvent.GUIEvent.Element = NULL;
	buttonEvent.GUIEvent.EventType = EGET_BUTTON_CLICKED;
	pressedStart = rect<s32>(0,0,0,0);
}
	
void BeautifulGUIButton::draw(){
	if(isVisible()){
		if(absPosDirty){updateAbsolutePosition();}
		updateScrollSpeed();
		IGUISkin* skin = Environment->getSkin();
		if(pressed){
			skin->draw3DButtonPanePressed(this, AbsoluteRect, &AbsoluteClippingRect);
		}else{
			skin->draw3DButtonPaneStandard(this, AbsoluteRect, &AbsoluteClippingRect);
		}
		IAggregatableGUIElement::draw();//draw children etc (don't draw the aggregation)
	}
}

bool BeautifulGUIButton::OnEvent(const irr::SEvent& event){
	if(isEnabled() && event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		vector2d<s32> mPos(m.X,m.Y);
		if(pressed && m.Event==EMIE_LMOUSE_LEFT_UP){
			pressed = false;
			if(AbsoluteRect.isPointInside(mPos) && pressedStart.isPointInside(mPos)){
				if(Parent && isTrulyVisible()){
					AggregateGUIElement::OnEvent(event);//because parent may decide to delete itself or this element on button event
					irr::SEvent buttonEventCopy = buttonEvent;//make a copy because "this" may be deleted
					Parent->OnEvent(buttonEventCopy);
					return true;
				}
			}
		}else if(pressed && m.Event==EMIE_MOUSE_MOVED){
			pressed = AbsoluteRect.isPointInside(mPos) && pressedStart.isPointInside(mPos);
		}else if(!pressed && m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			pressed = AbsoluteRect.isPointInside(mPos);
			pressedStart = AbsoluteRect;
		}
	}
	return AggregateGUIElement::OnEvent(event);
}
