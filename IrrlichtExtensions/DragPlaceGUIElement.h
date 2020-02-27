#ifndef DragPlaceGUIElement_H_INCLUDED
#define DragPlaceGUIElement_H_INCLUDED

#include "IAggregatableGUIElement.h"

class DraggableGUIElement;

//! Represents a place where a DraggableGUIElement can be dragged to/from.
class DragPlaceGUIElement : public IAggregatableGUIElement{

	protected:
	
	DraggableGUIElement* child;
	irr::gui::IGUIElement* placeholder;

	public:
	
	//! placeholder: if NULL recommended space becomes 0 if no child available otherwise placeholder becomes visible if no child available
	//! Important: Places which can be empty (==targets) must have a placeholder, otherwise drag and drop doesn't work (can be an EmptyGUIElement also)
	DragPlaceGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, irr::s32 id, irr::gui::IGUIElement* placeholder = NULL, DraggableGUIElement* child = NULL, void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	virtual ~DragPlaceGUIElement(){}
	
	void removeChild();
	
	//! sets a child only if the current child is NULL
	void setChild(DraggableGUIElement* child);
	
	DraggableGUIElement* getChild();
	
	void updateAbsolutePosition();
	
	irr::f32 getRecommendedSpace();
	
};

#endif
