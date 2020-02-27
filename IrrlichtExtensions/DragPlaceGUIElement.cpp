#include "DragPlaceGUIElement.h"
#include "DraggableGUIElement.h"

#include <IGUIEnvironment.h>

using namespace irr;
using namespace core;

DragPlaceGUIElement::DragPlaceGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, irr::s32 id, irr::gui::IGUIElement* placeholder, DraggableGUIElement* child, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, aspectRatio, recommendedSpace, aspectRatio, maintainAspectRatio, false, id, data, parent, rectangle),
	child(child),
	placeholder(placeholder){
	if(child){addChild(child);}
	if(placeholder){
		addChild(placeholder);
		placeholder->setVisible(child==NULL);
	}
	setName("DragPlaceGUIElement");
}

void DragPlaceGUIElement::removeChild(){
	if(child){
		IAggregatableGUIElement::removeChild(child);
		child = NULL;
		if(placeholder){placeholder->setVisible(true);}
		Parent->updateAbsolutePosition();
	}
}
	
void DragPlaceGUIElement::setChild(DraggableGUIElement* child){
	if(this->child==NULL){
		addChild(child);
		this->child = child;
		if(placeholder){placeholder->setVisible(child==NULL);}
		Parent->updateAbsolutePosition();
	}
}

DraggableGUIElement* DragPlaceGUIElement::getChild(){
	return child;
}

void DragPlaceGUIElement::updateAbsolutePosition(){
	if(child!=NULL || placeholder!=NULL){
		(child?child:placeholder)->setRelativePosition(rect<s32>(0,0,RelativeRect.getWidth(),RelativeRect.getHeight()));
	}
	IAggregatableGUIElement::updateAbsolutePosition();
}

irr::f32 DragPlaceGUIElement::getRecommendedSpace(){
	if(child==NULL && placeholder==NULL){
		return 0.f;
	}
	return IAggregatableGUIElement::getRecommendedSpace();
}
