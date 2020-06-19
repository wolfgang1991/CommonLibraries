#ifndef BeautifulGUIImage_H_INCLUDED
#define BeautifulGUIImage_H_INCLUDED

#include "IAggregatableGUIElement.h"

#include <cstdint>

class Drawer2D;

class BeautifulGUIImage : public IAggregatableGUIElement{

	private:
	
	irr::video::ITexture* tex;
	Drawer2D* drawer;
	
	irr::video::SColor colors[4];
	
	public:
	
	//! pointers must not be NULL (except tex, tex may be NULL)
	BeautifulGUIImage(Drawer2D* drawer, irr::video::ITexture* tex, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, bool maintainAspectRatio, irr::s32 id, irr::video::SColor color = irr::video::SColor(255,255,255,255), void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	void draw();
	
	//! set color for seperate vertices i==0 => upper left, i increases clockwise
	void setColor(uint32_t i, irr::video::SColor color);
	
	//! set color for all vertices
	void setColor(irr::video::SColor color);
	
	void setTexture(irr::video::ITexture* tex);
	
};

#endif
