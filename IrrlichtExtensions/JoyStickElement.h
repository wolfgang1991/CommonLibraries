#ifndef JoyStickElement_H_
#define JoyStickElement_H_

#include "IAggregatableGUIElement.h"

#include <cstdint>

class Drawer2D;

class JoyStickElement : public IAggregatableGUIElement{

	irr::video::ITexture* background;
	irr::video::ITexture* handle;
	Drawer2D* drawer;
	
	irr::core::vector2d<irr::s32> offset;
	irr::core::vector2d<irr::s32> startMousePos;
	bool moving;
	
	irr::core::rect<irr::s32> finalRect;
	
	irr::f32 usableArea;

	public:
	
	JoyStickElement(Drawer2D* drawer, irr::gui::IGUIEnvironment* environment, irr::video::ITexture* background, irr::video::ITexture* handle, irr::f32 recommendedSpace, bool maintainAspectRatio, irr::s32 id, irr::video::SColor color = irr::video::SColor(255,255,255,255), void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	virtual ~JoyStickElement(){}
	
	bool OnEvent(const irr::SEvent& event) override;
	
	void draw() override;
	
	//! center 0,0, upperleft: -1,-1, lowerright: 1,1
	irr::core::vector2d<irr::f32> getJoystickPosition() const;
	
	//! 0-1 for 0-100%
	void setUsableArea(irr::f32 usableArea);
	
};

#endif
