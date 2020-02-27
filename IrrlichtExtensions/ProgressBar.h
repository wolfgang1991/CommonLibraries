#ifndef ProgressBar_H_INCLUDED
#define ProgressBar_H_INCLUDED

#include "IAggregatableGUIElement.h"

#include <rect.h>

class Drawer2D;

class ProgressBar : public IAggregatableGUIElement{

	private:
	
	irr::f32 pos;
	bool drawText;
	
	double sqrtArea;
	
	bool overrideColorEnabled;
	irr::video::SColor overrideColor;
	
	Drawer2D* drawer;
	
	public:
	
	ProgressBar(Drawer2D* drawer, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, bool drawText, irr::s32 id, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
		
	void setPos(irr::f32 pos);
	
	irr::f32 getPos();
	
	void draw();
	
	//! NULL to disable override color
	void setOverrideColor(irr::video::SColor* color = NULL);
};

#endif
