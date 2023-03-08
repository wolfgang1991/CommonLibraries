#include "TouchKey.h"
#include "ICommonAppContext.h"
#include "timing.h"
#include "FlexibleFont.h"
#include "mathUtils.h"
#include "Drawer2D.h"

#include <IrrlichtDevice.h>

using namespace std;
using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

#define PRESS_SCALE 0.75

TouchKey::TouchKey(ICommonAppContext* context, wchar_t C, wchar_t CShifted, irr::EKEY_CODE Keycode, irr::core::rect<s32> Rectangle, irr::video::ITexture* OverrideTex){
	ch = C;
	cString = stringw(&C, 1);
	cShiftedString = stringw(&CShifted, 1);
	cShifted = CShifted;
	keycode = Keycode;
	overrideTex = OverrideTex;
	event.Char = ch;
	event.Control = false;
	event.Key = Keycode;
	event.PressedDown = false;
	event.Shift = false;
	rectangle = Rectangle;
	c = context;
	drawer = c->getDrawer();
	device = c->getIrrlichtDevice();
	driver = device->getVideoDriver();
	env = device->getGUIEnvironment();
	wasPressedOutside = false;
	T0 = 1.0; t0 = 0.5; m = 0.1; lastPressedEventTime = lastPressedTime = 0.0;
}

void TouchKey::reset(){
	wasPressedOutside = false;
	event.PressedDown = false;
	lastPressedTime = lastPressedEventTime = 0.0;
}

static inline bool isPointInsideExcludingLowerRight(const rect<s32>& r, const vector2d<s32>& v){
	return v.X>=r.UpperLeftCorner.X && v.X<r.LowerRightCorner.X && v.Y>=r.UpperLeftCorner.Y && v.Y<r.LowerRightCorner.Y;
}

bool TouchKey::processMouseEvent(irr::SEvent::SMouseInput mevent, bool* isMouseInside){
	vector2d<s32> mpos(mevent.X, mevent.Y);
	bool mInside = isPointInsideExcludingLowerRight(rectangle, mpos);
	if(isMouseInside){*isMouseInside = mInside;}
	if(mInside){
		if(mevent.Event == EMIE_LMOUSE_PRESSED_DOWN){
			event.PressedDown = true;
			lastPressedEventTime = 0.0;
			lastPressedTime = getSecs();
			return true;
		}else if(mevent.Event == EMIE_LMOUSE_LEFT_UP){
			wasPressedOutside = false;
			event.PressedDown = false;
			return true;
		}
	}else{
		bool ret = false;
		if(event.PressedDown){ret = true;}
		event.PressedDown = false;
		wasPressedOutside = mevent.isLeftPressed();
		return ret;
	}
	return false;
}

bool TouchKey::mousePressedDownOutside(){
	return wasPressedOutside;
}

bool TouchKey::render(FlexibleFont* font, bool drawBox, SColor color){
	if(event.PressedDown){color.setAlpha(255); color.setRed(PRESS_SCALE*color.getRed()); color.setGreen(PRESS_SCALE*color.getGreen()); color.setBlue(PRESS_SCALE*color.getBlue());}
	SColor halfColor(255, color.getRed()/2, color.getGreen()/2, color.getBlue()/2);
	if(drawBox){
		driver->draw2DRectangle(color, rectangle);
		driver->draw2DRectangleOutline(rect<s32>(rectangle.UpperLeftCorner.X+1,rectangle.UpperLeftCorner.Y+1,rectangle.LowerRightCorner.X-1,rectangle.LowerRightCorner.Y-1), halfColor);
	}
	if(overrideTex){
		dimension2d<u32> tsize = overrideTex->getOriginalSize();
		rect<s32> texrect = makeXic(rectangle, (float)tsize.Width/(float)tsize.Height);
		drawer->setTextureWrap(ETC_CLAMP, ETC_CLAMP);
		drawer->draw(overrideTex, texrect.UpperLeftCorner, texrect.getSize());
		drawer->setTextureWrap();
	}else{
		if(!event.Shift){
			font->draw(cString, rectangle, SColor(255, 0, 0, 0), true, true, &rectangle);
			if(!(ch>=L'a' && ch<=L'z')){
				rect<s32> greyRect(rectangle);
				greyRect.LowerRightCorner.Y = (greyRect.UpperLeftCorner.Y+greyRect.LowerRightCorner.Y)/2;
				greyRect.LowerRightCorner.X = (greyRect.UpperLeftCorner.X+greyRect.LowerRightCorner.X)/2;
				irr::f32 optimalScale = font->calculateOptimalScale(cShiftedString.c_str(), dimension2d<u32>(greyRect.getWidth(), greyRect.getHeight()));
				vector2d<f32> currentScale = font->getDefaultScale();
				font->setDefaultScale(vector2d<f32>(optimalScale,optimalScale));
				font->draw(cShiftedString, greyRect, halfColor, true, true, &greyRect);
				font->setDefaultScale(currentScale);
			}
		}else{
			font->draw(cShiftedString, rectangle, SColor(255, 0, 0, 0), true, true, &rectangle);
		}
	}
	if(event.PressedDown){//automatically create event for pressed key in case of a longer tap (with increasing frequency)
		double t = getSecs()-lastPressedTime;
		double T = Clamp(T0-((t-t0)*((T0-m)/(2.0*t0))), m, T0);
		if(t-lastPressedEventTime>=T){
			lastPressedEventTime = t;
			return true;
		}
	}
	return false;
}

irr::SEvent::SKeyInput TouchKey::getEvent(){
	return event;
}

void TouchKey::setOverrideTexture(irr::video::ITexture* OverrideTex){
	overrideTex = OverrideTex;
}

irr::video::ITexture* TouchKey::getOverrideTexture(){
	return overrideTex;
}

void TouchKey::setShiftPressed(bool pressed){
	event.Shift = pressed;
	event.Char = pressed?cShifted:ch;
}

void TouchKey::setRectangle(const irr::core::rect<irr::s32>& rectangle){
	this->rectangle = rectangle;
}
