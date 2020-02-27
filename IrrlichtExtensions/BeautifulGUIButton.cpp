#include <BeautifulGUIButton.h>

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
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		vector2d<s32> mPos(m.X,m.Y);
		if(pressed && m.Event==EMIE_LMOUSE_LEFT_UP){
			pressed = false;
			if(AbsoluteRect.isPointInside(mPos)){
				if(!AggregateGUIElement::OnEvent(event)){//event must be finished before issuing the button event which may lead to the destruction of the BeautifulGUIButton
					Parent->OnEvent(buttonEvent);
					return true;
				}
			}
		}else if(!pressed && m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			pressed = AbsoluteRect.isPointInside(mPos);
		}
	}
	return AggregateGUIElement::OnEvent(event);
}
