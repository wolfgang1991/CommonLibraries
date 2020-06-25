#include "DraggableGUIElement.h"
#include "DragPlaceGUIElement.h"

#include <iostream>

using namespace irr;
using namespace core;
using namespace video;

//TODO: override elemente für dragging und platziert definieren
//TODO: skin extensions für markierungen von PRESSED DraggableGUIElements und gleiches für verfügare DragPlaces (einblenden sobald DRAGGING)

DraggableGUIElement::DraggableGUIElement(irr::gui::IGUIEnvironment* environment, irr::s32 id, const irr::core::dimension2d<irr::u32> moveSize, DragPlaceGUIElement* origin, std::list<DragPlaceGUIElement*> targets, vector2d<u32> scrollThreshold, irr::gui::IGUIElement* child, irr::gui::IGUIElement* childWhileDragging, irr::gui::IGUIElement* childAtTarget, IDraggableCallback* cbk):
	irr::gui::IGUIElement(irr::gui::EGUIET_ELEMENT, environment, origin, id, rect<s32>(0,0,origin->getRelativePosition().getWidth(), origin->getRelativePosition().getHeight())),
	origin(origin),
	targets(targets),
	state(NOTHING),
	storedMousePos(0,0),
	scrollThreshold(scrollThreshold),
	moveSize(moveSize),
	child(child),
	childWhileDragging(childWhileDragging),
	childAtTarget(childAtTarget),
	cbk(cbk){
	addChild(child);
	if(childWhileDragging){addChild(childWhileDragging);}
	if(childAtTarget){addChild(childAtTarget);}
	origin->setChild(this);
	drop();
	setName("DraggableGUIElement");
	setChildrenVisibility();
}

DragPlaceGUIElement* DraggableGUIElement::getOrigin(){
	return origin;
}
	
void DraggableGUIElement::setScrollThreshold(irr::core::vector2d<irr::u32> scrollThreshold){
	this->scrollThreshold = scrollThreshold;
}

irr::gui::IGUIElement* DraggableGUIElement::selectChild(){
	if(state==DRAGGING){
		return childWhileDragging?childWhileDragging:child;
	}else if((DragPlaceGUIElement*)Parent==origin){
		return child;
	}else{
		return childAtTarget?childAtTarget:child;
	}
}

void DraggableGUIElement::setChildrenVisibility(){
	child->setVisible(false);
	if(childWhileDragging){childWhileDragging->setVisible(false);}
	if(childAtTarget){childAtTarget->setVisible(false);}
	selectChild()->setVisible(true);
}
	
bool DraggableGUIElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		vector2d<s32> mPos(m.X, m.Y);
		if(m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			storedMousePos = mPos;
			state = PRESSED;
		}else if(m.Event==EMIE_MOUSE_MOVED){
			vector2d<s32> delta = mPos-storedMousePos;
			if(state==PRESSED){
				if((u32)abs(delta.X)>scrollThreshold.X || (u32)abs(delta.Y)>scrollThreshold.Y){//TODO: threshold lieber über den Platz definieren
					state = DRAGGING;
					setChildrenVisibility();
					updateAbsolutePosition();
					if(cbk){cbk->OnDrag(this);}
					return true;
				}
			}else if(state==DRAGGING){
				vector2d<s32> offset(moveSize.Width/2, moveSize.Height/2);
				IGUIElement* schild = selectChild();
				schild->setRelativePosition(rect<s32>(mPos-offset-AbsoluteRect.UpperLeftCorner, mPos+offset-AbsoluteRect.UpperLeftCorner));
				schild->updateAbsolutePosition();
				schild->setNotClipped(true);
				return true;
			}
		}else if(m.Event==EMIE_LMOUSE_LEFT_UP){
			if(state==DRAGGING){
				bool dropped = false;
				for(auto it = targets.begin(); it != targets.end(); ++it){
					DragPlaceGUIElement* place = *it;
					if(place->isTrulyVisible() && place->getAbsolutePosition().isPointInside(mPos)){
						if(place!=Parent){
							moveTo(place);
						}
						dropped = true;
						break;
					}
				}
				if(!dropped){
					moveToOrigin();
				}
			}
			selectChild()->setNotClipped(false);
			state = NOTHING;
			setChildrenVisibility();
			Parent->updateAbsolutePosition();
		}
	}
	return irr::gui::IGUIElement::OnEvent(event);
}

void DraggableGUIElement::moveTo(DragPlaceGUIElement* place){
	selectChild()->setNotClipped(false);
	state = NOTHING;
	if(Parent!=place){
		grab();
		((DragPlaceGUIElement*)Parent)->removeChild();
		DraggableGUIElement* toReplace = place->getChild();
		if(toReplace){toReplace->moveToOrigin();}
		place->setChild(this);
		drop();
	}
	if(cbk){cbk->OnDrop(place, this);}
	setChildrenVisibility();
}

void DraggableGUIElement::moveToOrigin(){
	selectChild()->setNotClipped(false);
	state = NOTHING;
	if(Parent!=origin){
		grab();
		((DragPlaceGUIElement*)Parent)->removeChild();
		origin->removeChild();
		origin->setChild(this);
		drop();
	}
	if(cbk){cbk->OnMoveToOrigin(origin, this);}
	setChildrenVisibility();
}

void DraggableGUIElement::updateAbsolutePosition(){
	if(state!=DRAGGING){
		recalculateAbsolutePosition(false);
		for(irr::core::list<IGUIElement*>::Iterator it = Children.begin(); it != Children.end(); ++it){
			(*it)->setRelativePosition(irr::core::rect<irr::s32>(0,0,AbsoluteRect.getWidth(),AbsoluteRect.getHeight()));
			(*it)->updateAbsolutePosition();
		}
	}
}

