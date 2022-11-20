#ifndef AggregateGUIElement_H_INCLUDED
#define AggregateGUIElement_H_INCLUDED

#include "IAggregatableGUIElement.h"
#include "IScrollable.h"

#include <vector>

//! GUI Element for aggregating other GUI Elements vertically or horizontally, the relative position of the children is overwritten when applying the constraints of the aggregation
class AggregateGUIElement : public IAggregatableGUIElement, public IScrollable{

	protected:
	
	std::vector<IAggregatableGUIElement*> subElements;
	std::vector<IAggregatableGUIElement*> activatedSubElements;
	bool isHorizontal;
	bool isScrollable;
	
	int scrollState;//0: nothing, 1: scrolling, 2: deccelerating
	irr::core::vector2d<irr::s32> storedMousePos;
	irr::s32 storedScrollPos;//from mousedown event
	irr::s32 scrollPos;
	
	irr::s32 lastScrollPos;//from last move event
	double scrollSpeed;
	double lastTime;
	bool isMouseDown;
	//bool lastMoved;//true if mouse position changed in the last iteration
	double stopScrollSpeed;//px/s, speed at which scrollSpeed is set to 0
	
	double releaseTime;
	
	irr::s32 maxScrollPos, maxScrollPosActive;
	
	bool multiSelect;
	
	//! Updates the Sub Element's position (rectangles) based on the constraints defined by the parameters of this
	void updateSubElementsPosition(const std::vector<IAggregatableGUIElement*>& se, irr::s32& maxScrollPosOut);
	
	//! updates the scroll speed using the last and current scroll positions and updates the scroll position if in decellerating state
	void updateScrollSpeed();
	
	bool absPosDirty;

	public:
	
	//! init with list of children + set their parent
	//! isHorizontal: true if horizontal aggregation, false if vertical
	//! isScrollable: true if space recommendation of children is interpreted as part of the available space (children may not fit), false if space recommendation is interpreted as a weight for distributing the available space
	//! subElements: Kinder die im Normalzustand verwendet werden
	//! activatedSubElements: Kinder die im aktivierten Zustand verwendet werden, falls leer gibt es nur den Normalzustand
	AggregateGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, irr::f32 recommendedActiveSpace, irr::f32 activeAspectRatio, bool maintainAspectRatio, bool isHorizontal, bool isScrollable, const std::initializer_list<IAggregatableGUIElement*>& subElements, const std::initializer_list<IAggregatableGUIElement*>& activatedSubElements, bool isActivateAble, irr::s32 id, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	//! subElement must not be NULL
	//! don't forget to call drop on the subElement if you don't want to hold an extra reference to it after creating and adding
	void addSubElement(IAggregatableGUIElement* subElement);
	
	//! subElement must not be NULL
	//! don't forget to call drop on the subElement if you don't want to hold an extra reference to it after creating and adding
	void addActivatedSubElement(IAggregatableGUIElement* subElement);
	
	irr::u32 getActivatedSubElementCount() const;
	
	irr::u32 getSubElementCount() const;
	
	IAggregatableGUIElement* getSubElement(irr::u32 i);
	
	//! O(1)
	void removeSubElement(irr::u32 i);
	
	//! Delete all (active) sub elements O(n)
	void clear();
	
	IAggregatableGUIElement* getActivatedSubElement(irr::u32 i);
	
	//! O(n)
	void removeActivatedSubElement(irr::u32 i);

	//! recalculates absolute position of this element, updates relative and absolute positions of the child elements based on the constraints
	//@sa irr::gui::IGUIElement::updateAbsolutePosition()
	void updateAbsolutePosition();
	
	bool OnEvent(const irr::SEvent& event);
	
	void draw();
	
	void setActivationLock(bool on);
	
	//! true if multiple children can be activated
	void setMultiSelectable(bool multiSelect);
	
	bool getMultiSelectable();
	
	void OnScrollBarChanged(ScrollBar* bar);
	
	//! pos \in [0-1]
	void setScrollPosition(irr::f32 pos, bool notifyScrollbar = true);
	
	//! result \in [0-1]
	irr::f32 getScrollPosition() const;
	
	void setActive(bool active);
	
	//! only applicable if only one sub element selectable, returns the index of the sub element and fills outGUIElement if outGUIElement!=NULL
	//! -1 if no selection found (in case of multi select: the first selection is returned which makes usually no sense)
	int32_t getSingleSelected(IAggregatableGUIElement** outGUIElement = NULL);
	
	irr::f32 getScrollStepSize() const;
	
	bool isTrulyScrollable() const;

};

#endif
