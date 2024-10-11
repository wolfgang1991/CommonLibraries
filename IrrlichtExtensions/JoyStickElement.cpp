#include "JoyStickElement.h"
#include "Drawer2D.h"
#include "utilities.h"
#include "mathUtils.h"

#include <ITexture.h>
#include <IVideoDriver.h>
#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

static f32 calcTexAspectRatio(ITexture* tex){
	return tex!=NULL?((irr::f32)tex->getOriginalSize().Width)/((irr::f32)tex->getOriginalSize().Height):1.f;
}

JoyStickElement::JoyStickElement(Drawer2D* drawer, irr::gui::IGUIEnvironment* environment, irr::video::ITexture* background, irr::video::ITexture* handle, irr::f32 recommendedSpace, bool maintainAspectRatio, irr::s32 id, irr::video::SColor color, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, calcTexAspectRatio(background), recommendedSpace, calcTexAspectRatio(background), maintainAspectRatio, false, id, data, parent, rectangle),
	background(background),
	handle(handle),
	drawer(drawer),
	offset(0,0),
	startMousePos(0,0),
	moving(false),
	finalRect(0,0,1,1){
	usableArea = 0.65f;
	setName("JoyStickElement");
}

void JoyStickElement::setUsableArea(irr::f32 usableArea){
	this->usableArea = usableArea;
}

void JoyStickElement::draw(){
	if(isVisible() && background!=NULL && handle!=NULL){
		IVideoDriver* driver = drawer->getDevice()->getVideoDriver();
		rect<s32> vp = driver->getViewPort();
		drawer->setTextureWrap(ETC_CLAMP, ETC_CLAMP);
		driver->setViewPort(AbsoluteClippingRect);
		finalRect = maintainAspectRatio?makeXic(AbsoluteRect, aspectRatio):AbsoluteRect;
		drawer->draw(background, rect<s32>(finalRect.UpperLeftCorner-AbsoluteClippingRect.UpperLeftCorner, finalRect.LowerRightCorner-AbsoluteClippingRect.UpperLeftCorner));
		driver->setViewPort(vp);
		drawer->draw(handle, rect<s32>(finalRect.UpperLeftCorner-vp.UpperLeftCorner+offset, finalRect.LowerRightCorner-vp.UpperLeftCorner+offset));
		drawer->setTextureWrap();
		IAggregatableGUIElement::draw();//draw children etc
	}
}

bool JoyStickElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		if(moving){
			if(m.Event==EMIE_MOUSE_MOVED){
				offset = vector2d<s32>(m.X,m.Y) - startMousePos;
			}else if(m.Event==EMIE_LMOUSE_LEFT_UP){
				moving = false;
				offset = vector2d<s32>(0,0);
			}
		}else if(m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			startMousePos = vector2d<s32>(m.X,m.Y);
			vector2d<s32> delta = startMousePos-AbsoluteRect.getCenter();
			moving = std::abs(delta.X)<0.5f*usableArea*AbsoluteRect.getWidth() && std::abs(delta.Y)<0.5f*usableArea*AbsoluteRect.getHeight();
		}
		return true;
	}
	return false;
}

irr::core::vector2d<irr::f32> JoyStickElement::getJoystickPosition() const{
	if(moving){
		return irr::core::vector2d<irr::f32>((f32)offset.X/(0.5f*finalRect.getWidth()), (f32)offset.Y/(0.5f*finalRect.getHeight()));
	}else{
		return irr::core::vector2d<irr::f32>(0,0);
	}
}
