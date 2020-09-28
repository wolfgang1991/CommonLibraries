#ifndef BeautifulGUIButton_H_INCLUDED
#define BeautifulGUIButton_H_INCLUDED

#include "AggregateGUIElement.h"

//! A BeautifulGUIButton is a non activable Aggregation which emits button events, it also uses the regular skin functions for button drawing. Therefore the usual button styles apply.
//! The aggregation does not have any border by itself and the id can be used to define a button style.
class BeautifulGUIButton : public AggregateGUIElement{

	protected:
	
	bool pressed;
	irr::core::rect<irr::s32> pressedStart;
	
	irr::SEvent buttonEvent;
	
	public:
	
	//! id is NOT used for fetching a ISkinExtension here (so arbitrary values are possible)
	BeautifulGUIButton(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, bool isHorizontal, bool isScrollable, const std::initializer_list<IAggregatableGUIElement*>& subElements, const std::initializer_list<IAggregatableGUIElement*>& activatedSubElements, irr::s32 id, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	void draw();
	
	bool OnEvent(const irr::SEvent& event);
	
};

#endif
