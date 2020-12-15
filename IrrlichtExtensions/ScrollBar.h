#ifndef ScrollBar_H_INCLUDED
#define ScrollBar_H_INCLUDED

#include "IAggregatableGUIElement.h"

#include <rect.h>

#define SCROLL_WHEEL_PART 0.25f//defines how much a scroll wheel tick shall influence the scroll position 0.25f==25%

class IScrollable;
class AggregateGUIElement;

class ScrollBar : public IAggregatableGUIElement{

	private:
	
	bool isHorizontal;
	IScrollable* scrollable;
	irr::f32 pos;
	
	double sqrtArea;
	irr::s32 handleSize;
	
	irr::core::rect<irr::s32> handleRect;
	irr::s32 scrollSpace;//available space in px on the scrollbar
	
	bool scrolling;
	irr::core::vector2d<irr::s32> storedMPos;
	irr::f32 storedPos;
	
	public:
	
	ScrollBar(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, bool isHorizontal, irr::s32 id, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	~ScrollBar();
	
	void updateAbsolutePosition();
	
	bool OnEvent(const irr::SEvent& event);
	
	void linkToScrollable(IScrollable* scrollable);
	
	void unlinkScrollable();
	
	//! pos in [0,1]; notifyLinks: true if linked scrollable shall be notified to adjust its scroll position
	void setPos(irr::f32 pos, bool notifyLinks);
	
	irr::f32 getPos();
	
	void draw();
};

#endif
