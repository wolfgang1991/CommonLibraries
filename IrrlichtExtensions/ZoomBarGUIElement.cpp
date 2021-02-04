#include "ZoomBarGUIElement.h"

#include <Drawer2D.h>
#include <mathUtils.h>
#include <FlexibleFont.h>

#include <IGUIEnvironment.h>

#include <iostream>

#define ZOOM_SPEED .75f //in parts per bar height

using namespace irr;
using namespace core;
using namespace gui;
using namespace video;

ZoomBarGUIElement::ZoomBarGUIElement(const std::function<void(ZoomState,irr::f32)>& zoomCallback, Drawer2D* drawer, irr::gui::IGUIEnvironment* environment, irr::video::ITexture* zoomNormal, irr::video::ITexture* zoomPressed, irr::gui::IGUIElement* parent, const rect<s32>& rectangle, irr::s32 id):
	IGUIElement(irr::gui::EGUIET_ELEMENT, environment, parent!=NULL?parent:(environment->getRootGUIElement()), id, rectangle),
	zoomCallback(zoomCallback), drawer(drawer), zoomNormal(zoomNormal),zoomPressed(zoomPressed){
	zoomState = RELEASED;
	moveDelta = 0.01*environment->getVideoDriver()->getScreenSize().Height;
	zoom = zoomAtPressed = 1.f;
	drop();//reference is hold by parent
	zoomCallback(zoomState, zoom);
}

void ZoomBarGUIElement::draw(){
	if(isVisible()){
		drawer->setTextureWrap(ETC_CLAMP, ETC_CLAMP);
		drawer->draw(zoomState==RELEASED?zoomNormal:zoomPressed, AbsoluteRect, NULL);
		drawer->setTextureWrap();
		//Draw +/- labels
		rect<s32> plusRect(AbsoluteRect.UpperLeftCorner.X, AbsoluteRect.UpperLeftCorner.Y, AbsoluteRect.LowerRightCorner.X, Min(AbsoluteRect.UpperLeftCorner.Y+AbsoluteRect.getWidth(), AbsoluteRect.LowerRightCorner.Y));
		rect<s32> minusRect(AbsoluteRect.UpperLeftCorner.X, Max(AbsoluteRect.LowerRightCorner.Y-AbsoluteRect.getWidth(), AbsoluteRect.UpperLeftCorner.Y), AbsoluteRect.LowerRightCorner.X, AbsoluteRect.LowerRightCorner.Y);
		FlexibleFont* font = (FlexibleFont*)(Environment->getSkin()->getFont());
		auto scale = font->getDefaultScale();
		f32 optimalScale = font->calculateOptimalScale(L"+", dimension2d<u32>(7*plusRect.getWidth()/8, 7*plusRect.getHeight()/8));
		font->setDefaultScale(vector2d<f32>(optimalScale,optimalScale));
		f32 borderSize = font->getDefaultBorderSize();
		font->setDefaultBorderSize(.05f);
		auto borderColor = font->getDefaultBorderColor();
		font->setDefaultBorderColor(SColor(96,0,0,0));
		font->draw(L"+", plusRect, SColor(96,255,255,255), true, true, &plusRect);
		font->draw(L"-", minusRect, SColor(96,255,255,255), true, true, &minusRect);
		font->setDefaultBorderColor(borderColor);
		font->setDefaultBorderSize(borderSize);
		font->setDefaultScale(scale);
		IGUIElement::draw();
	}
}

bool ZoomBarGUIElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		vector2d<s32> mPos(m.X,m.Y);
		if(AbsoluteRect.isPointInside(mPos) || zoomState!=RELEASED){//zoom events
			if(m.Event == EMIE_LMOUSE_PRESSED_DOWN){
				zoomState = PRESSED;
				mousePressedPos = mPos;
				zoomAtPressed = zoom;
				zoomCallback(zoomState, zoom);
			}else if(m.Event == EMIE_LMOUSE_LEFT_UP || !m.isLeftPressed()){
				if(zoomState==PRESSED){//on click apply constant zoom factor
					f32 zoomFactor = mPos.Y<AbsoluteRect.getCenter().Y?(1.f/ZOOM_SPEED):ZOOM_SPEED;
					zoom = Max(1.f, zoomAtPressed*zoomFactor);
				}
				zoomState = RELEASED;
				zoomCallback(zoomState, zoom);
			}else if(m.Event == EMIE_MOUSE_MOVED && zoomState!=RELEASED){
				s32 delta = mPos.Y-mousePressedPos.Y;
				if(abs(delta)>moveDelta){
					zoomState = ZOOMING;
				}
				if(zoomState==ZOOMING){//apply zooming
					zoom = Max(1.f, zoomAtPressed * (1.f + ZOOM_SPEED*((f32)-delta)/(f32)AbsoluteRect.getHeight()));
				}
				zoomCallback(zoomState, zoom);
			}
		}
	}
	return IGUIElement::OnEvent(event);
}
