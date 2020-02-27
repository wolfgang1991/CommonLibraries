#ifndef TOUCHKEY_H_INCLUDED
#define TOUCHKEY_H_INCLUDED

#include "ForwardDeclarations.h"

#include <rect.h>
#include <SColor.h>
#include <Keycodes.h>
#include <IEventReceiver.h>

class FlexibleFont;
class ICommonAppContext;
class Drawer2D;

class TouchKey{

	public:

	TouchKey(ICommonAppContext* context, wchar_t C, wchar_t CShifted, irr::EKEY_CODE Keycode, irr::core::rect<irr::s32> Rectangle, irr::video::ITexture* OverrideTex = NULL);

	//! returns true if event shall be emitted; value of isMouseInside is independent of the SKeyInput event
	bool processMouseEvent(irr::SEvent::SMouseInput mevent, bool* isMouseInside = NULL);

	//! renders and updates the key. Returns true if event shall be emitted.
	bool render(FlexibleFont* font, bool drawBox, irr::video::SColor color);

	//! returns event which shall be emitted
	irr::SEvent::SKeyInput getEvent();

	void setOverrideTexture(irr::video::ITexture* OverrideTex);

	irr::video::ITexture* getOverrideTexture();

	//! needs to be called to switch to the "shift" function of the key
	void setShiftPressed(bool pressed);

	//! returns true if the mouse has been pressed down outside of the key
	bool mousePressedDownOutside();

	//! reset all states
	void reset();
	
	void setRectangle(const irr::core::rect<irr::s32>& rectangle);

	private:

	ICommonAppContext* c;
	Drawer2D* drawer;
	irr::IrrlichtDevice* device;
	irr::video::IVideoDriver* driver;
	irr::gui::IGUIEnvironment* env;
	wchar_t ch; irr::core::stringw cString;
	wchar_t cShifted; irr::core::stringw cShiftedString;
	irr::EKEY_CODE keycode;
	irr::core::rect<irr::s32> rectangle;
	irr::video::ITexture* overrideTex;

	double T0, t0, m, lastPressedTime;
	double lastPressedEventTime;//time since lastPressedTime since the last event has been determined

	irr::SEvent::SKeyInput event;

	bool wasPressedOutside;

};

#endif
