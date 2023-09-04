#include "BeautifulCheckBox.h"

#include <IGUISkin.h>
#include <IEventReceiver.h>
#include <ITimer.h>

using namespace irr;
using namespace gui;
using namespace core;

BeautifulCheckBox::BeautifulCheckBox(const wchar_t* text, irr::video::SColor color, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id, irr::f32 scale, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
		BeautifulGUIText(text, color, italicGradient,transformation, hcenter, vcenter, environment, recommendedSpace, id, scale, data, parent, rectangle){
	checked = false;
	event.EventType = EET_GUI_EVENT;
	event.GUIEvent.Caller = this;
	event.GUIEvent.Element = NULL;
	event.GUIEvent.EventType = EGET_CHECKBOX_CHANGED;
	pressedStart = rect<s32>(0,0,0,0);
	pressed = false;
}

BeautifulCheckBox::BeautifulCheckBox(const wchar_t* text, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id, irr::f32 scale, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
		BeautifulGUIText(text, italicGradient,transformation, hcenter, vcenter, environment, recommendedSpace, id, scale, data, parent, rectangle){
	checked = false;
	event.EventType = EET_GUI_EVENT;
	event.GUIEvent.Caller = this;
	event.GUIEvent.Element = NULL;
	event.GUIEvent.EventType = EGET_CHECKBOX_CHANGED;
	pressedStart = rect<s32>(0,0,0,0);
	pressed = false;
}
	
void BeautifulCheckBox::draw(){
	if(isVisible()){
		IGUISkin* skin = Environment->getSkin();
		const s32 height = skin->getSize(EGDS_CHECK_BOX_WIDTH);
		
		// the rectangle around the "checked" area.
		core::rect<s32> checkRect(AbsoluteRect.UpperLeftCorner.X, ((AbsoluteRect.getHeight() - height) / 2) + AbsoluteRect.UpperLeftCorner.Y, 0, 0);
		checkRect.LowerRightCorner.X = checkRect.UpperLeftCorner.X + height;
		checkRect.LowerRightCorner.Y = checkRect.UpperLeftCorner.Y + height;

		EGUI_DEFAULT_COLOR col = EGDC_GRAY_EDITABLE;
		if(isEnabled()){col = pressed ? EGDC_FOCUSED_EDITABLE : EGDC_EDITABLE;}
		skin->draw3DSunkenPane(this, skin->getColor(col), false, true, checkRect, &AbsoluteClippingRect);

		// the checked icon
		if(checked){
			skin->drawIcon(this, EGDI_CHECK_BOX_CHECKED, checkRect.getCenter(), 0, 0, false, &AbsoluteClippingRect);
		}
		
		auto tmp = AbsoluteRect;
		AbsoluteRect.UpperLeftCorner.X += height + 5;
		if(AbsoluteRect.UpperLeftCorner.X>AbsoluteRect.LowerRightCorner.X){AbsoluteRect.UpperLeftCorner.X = AbsoluteRect.LowerRightCorner.X;}
		BeautifulGUIText::draw();
		AbsoluteRect = tmp;
	}
}

bool BeautifulCheckBox::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		vector2d<s32> mPos(m.X,m.Y);
		if(pressed && m.Event==EMIE_LMOUSE_LEFT_UP){
			pressed = false;
			if(AbsoluteRect.isPointInside(mPos) && pressedStart.isPointInside(mPos)){
				if(Parent && isTrulyVisible()){
					checked = !checked;
					BeautifulGUIText::OnEvent(event);//important, otherwise e.g. parent aggregations don't scroll properly
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
	return BeautifulGUIText::OnEvent(event);
}

bool BeautifulCheckBox::isChecked() const{
	return checked;
}
	
void BeautifulCheckBox::setChecked(bool checked){
	this->checked = checked;
}
