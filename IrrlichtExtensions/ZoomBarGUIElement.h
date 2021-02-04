#ifndef ZoomBarGUIElement_H_INCLUDED
#define ZoomBarGUIElement_H_INCLUDED

#include <ForwardDeclarations.h>

#include <IGUIElement.h>

#include <functional>

class Drawer2D;

//! asserts that the used IGUIFont is a FlexibleFont (can safely be casted to FlexibleFont)
class ZoomBarGUIElement : public irr::gui::IGUIElement{

	public:
	
	enum ZoomState{
		RELEASED,
		PRESSED,
		ZOOMING,
		STATE_COUNT
	};

	private:
	
	std::function<void(ZoomState,irr::f32)> zoomCallback;
	
	ZoomState zoomState;
	irr::core::vector2d<irr::s32> mousePressedPos;
	irr::s32 moveDelta;//after moveDelta switch to ZOOMING
	
	irr::f32 zoomAtPressed, zoom;
	
	Drawer2D* drawer;
	
	irr::video::ITexture* zoomNormal;
	irr::video::ITexture* zoomPressed;

	public:
	
	ZoomBarGUIElement(const std::function<void(ZoomState,irr::f32)>& zoomCallback, Drawer2D* drawer, irr::gui::IGUIEnvironment* environment, irr::video::ITexture* zoomNormal, irr::video::ITexture* zoomPressed, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle, irr::s32 id = -1);
	
	void draw();
	
	bool OnEvent(const irr::SEvent& event);
	
	
};

#endif
