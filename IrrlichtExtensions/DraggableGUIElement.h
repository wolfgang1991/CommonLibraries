#ifndef DraggableGUIElement_H_INCLUDED
#define DraggableGUIElement_H_INCLUDED

#include <IGUIElement.h>

#include <list>

class DragPlaceGUIElement;
class DraggableGUIElement;

//! Interface for callbacks to process special cases for dragging and dropping
class IDraggableCallback{
	
	public:
	
	virtual ~IDraggableCallback(){}
	
	//! called when dragging starts
	virtual void OnDrag(DraggableGUIElement* ele){}
	
	//! called when ele dropped at place
	virtual void OnDrop(DragPlaceGUIElement* place, DraggableGUIElement* ele){}
	
	//! called when ele moved to origin
	virtual void OnMoveToOrigin(DragPlaceGUIElement* origin, DraggableGUIElement* ele){}
		
};

//! The DraggableGUIElement's parent should not be set manually. The parent must be a DragPlaceGUIElement.
class DraggableGUIElement : public irr::gui::IGUIElement{

	public:
	
	enum DragState{
		NOTHING,
		PRESSED,
		DRAGGING,
		STATE_COUNT
	};

	protected:
	
	DragPlaceGUIElement* origin;
	std::list<DragPlaceGUIElement*> targets;
	
	DragState state;
	irr::core::vector2d<irr::s32> storedMousePos;
	irr::core::vector2d<irr::u32> scrollThreshold;//different thresholds in x and y direction
	irr::core::dimension2d<irr::u32> moveSize;
	
	irr::gui::IGUIElement* child;
	irr::gui::IGUIElement* childWhileDragging;
	irr::gui::IGUIElement* childAtTarget;
	
	//! select correct child depending on state
	irr::gui::IGUIElement* selectChild();
	
	//! sets the children visibility depending on state
	void setChildrenVisibility();
	
	IDraggableCallback* cbk;

	public:
	
	//! origin must not be NULL, targets may be empty but must not be NULL, child makes no sense to be NULL
	//! moveSize: defines size in pixels during dragging
	//! defines the threshold for move distance which is used to identify dragging (in case the elements are in scrollable aggregation the direction of scrolling should have a very high threshold e.g ~(u32)0)
	DraggableGUIElement(irr::gui::IGUIEnvironment* environment, irr::s32 id, const irr::core::dimension2d<irr::u32> moveSize, DragPlaceGUIElement* origin, std::list<DragPlaceGUIElement*> targets, irr::core::vector2d<irr::u32> scrollThreshold, irr::gui::IGUIElement* child, irr::gui::IGUIElement* childWhileDragging=NULL, irr::gui::IGUIElement* childAtTarget=NULL, IDraggableCallback* cbk=NULL);
	
	virtual ~DraggableGUIElement(){}
	
	virtual bool OnEvent(const irr::SEvent& event);

	//void OnRemoveFromPlace();
	
	void updateAbsolutePosition();
	
	void moveToOrigin();
	
	//! place should be included in the targets list
	void moveTo(DragPlaceGUIElement* place);
	
	DragPlaceGUIElement* getOrigin();
	
	void setScrollThreshold(irr::core::vector2d<irr::u32> scrollThreshold);
	
};

#endif
