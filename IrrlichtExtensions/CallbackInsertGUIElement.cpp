#include "CallbackInsertGUIElement.h"

using namespace irr;
using namespace core;
using namespace gui;

CallbackInsertGUIElement::CallbackInsertGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, irr::f32 recommendedActiveSpace, irr::f32 activeAspectRatio, bool maintainAspectRatio, bool isActivateAble, const std::function<void()>& drawCbk, const std::function<bool(const irr::SEvent&)>& onEventCbk, const std::function<void(irr::u32)>& onPostRenderCbk, const std::function<void()>& onDeleteCbk, irr::s32 id, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, aspectRatio, recommendedActiveSpace, activeAspectRatio, maintainAspectRatio, isActivateAble, id, data , parent, rectangle),
	onEventCbk(onEventCbk), onPostRenderCbk(onPostRenderCbk), drawCbk(drawCbk), onDeleteCbk(onDeleteCbk){
}

CallbackInsertGUIElement::CallbackInsertGUIElement(irr::gui::IGUIEnvironment* environment, IGUIElement* parent, const std::function<void()>& drawCbk, const std::function<bool(const irr::SEvent&)>& onEventCbk, const std::function<void(irr::u32)>& onPostRenderCbk, const std::function<void()>& onDeleteCbk):
	IAggregatableGUIElement(environment, 1.f, 1.f, 1.f, 1.f, false, false, -1, NULL, parent, rect<s32>(0,0,parent->getRelativePosition().getWidth(),parent->getRelativePosition().getHeight())),
	onEventCbk(onEventCbk), onPostRenderCbk(onPostRenderCbk), drawCbk(drawCbk), onDeleteCbk(onDeleteCbk){
}

bool CallbackInsertGUIElement::OnEvent(const irr::SEvent& event){
	if(!onEventCbk(event)){
		return IAggregatableGUIElement::OnEvent(event);
	}
	return true;
}

void CallbackInsertGUIElement::OnPostRender(irr::u32 timeMs){
	onPostRenderCbk(timeMs);
	IAggregatableGUIElement::OnPostRender(timeMs);
}

void CallbackInsertGUIElement::draw(){
	if(isVisible()){
		drawCbk();
		IAggregatableGUIElement::draw();
	}
}

CallbackInsertGUIElement::~CallbackInsertGUIElement(){
	onDeleteCbk();
}

void CallbackInsertGUIElement::setDeleteCallback(const std::function<void()>& onDeleteCbk){
	this->onDeleteCbk = onDeleteCbk;
}
	
void CallbackInsertGUIElement::setDrawCallback(const std::function<void()>& drawCbk){
	this->drawCbk = drawCbk;
}

void CallbackInsertGUIElement::setOnPostRenderCallback(const std::function<void(irr::u32)>& onPostRenderCbk){
	this->onPostRenderCbk = onPostRenderCbk;
}

void CallbackInsertGUIElement::setOnEventCallback(const std::function<bool(const irr::SEvent&)>& onEventCbk){
	this->onEventCbk = onEventCbk;
}
