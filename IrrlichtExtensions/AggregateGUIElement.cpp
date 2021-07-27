#include <AggregateGUIElement.h>
#include <ScrollBar.h>
#include <mathUtils.h>
#include <AggregateSkinExtension.h>

#include <timing.h>

#include <IVideoDriver.h>
#include <IGUIEnvironment.h>
#include <IGUISkin.h>

#include <cstdint>
#include <iostream>

#define ACTIVATE_CLICK_THRESHOLD 0.01

using namespace irr;
using namespace gui;
using namespace core;

AggregateGUIElement::AggregateGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, irr::f32 recommendedActiveSpace, irr::f32 activeAspectRatio, bool maintainAspectRatio, bool isHorizontal, bool isScrollable, const std::initializer_list<IAggregatableGUIElement*>& subElements, const std::initializer_list<IAggregatableGUIElement*>& activatedSubElements, bool isActivateAble, s32 id, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, aspectRatio, recommendedActiveSpace, activeAspectRatio, maintainAspectRatio, isActivateAble, id, data, parent, rectangle),
	IScrollable(),
	subElements(subElements),
	activatedSubElements(activatedSubElements),
	isHorizontal(isHorizontal),
	isScrollable(isScrollable),
	scrollState(0),
	storedMousePos(0,0),
	storedScrollPos(0),
	scrollPos(0),
	lastScrollPos(0),
	scrollSpeed(0.0),
	lastTime(0.0),
	isMouseDown(false),
	//lastMoved(false),
	maxScrollPos(rectangle.getWidth()),
	maxScrollPosActive(maxScrollPos),
	multiSelect(false),
	absPosDirty(false)
{
	for(uint32_t i=0; i<this->subElements.size(); i++){
		addChild(this->subElements[i]);
	}
	for(uint32_t i=0; i<this->activatedSubElements.size(); i++){
		addChild(this->activatedSubElements[i]);
		this->activatedSubElements[i]->setVisible(false);
	}
	setName("AggregateGUIElement");
	dimension2d<u32> screenSize = environment->getVideoDriver()->getScreenSize();
	stopScrollSpeed = 0.02*sqrt(screenSize.Width*screenSize.Height);
	setScrollPosition(0, false);
}

void AggregateGUIElement::updateSubElementsPosition(const std::vector<IAggregatableGUIElement*>& se, irr::s32& maxScrollPosOut){
	f32 normFactor = 1.f;
	if(!isScrollable){
		f32 weightSum = 0.f;
		for(uint32_t i=0; i<se.size(); i++){
			weightSum += se[i]->getRecommendedSpace();
		}
		normFactor = weightSum>0.f?(1.f/weightSum):1.f;
	}
	f32 currentPos = 0.f;
	f32 totalSpace = (isHorizontal?AbsoluteRect.getWidth():AbsoluteRect.getHeight())-1;//space in the selected direction
	s32 totalOtherSpace = (isHorizontal?AbsoluteRect.getHeight():AbsoluteRect.getWidth())-1;//space in the other direction
	for(uint32_t i=0; i<se.size(); i++){
		IAggregatableGUIElement* ele = se[i];
		f32 delta = normFactor*ele->getRecommendedSpace()*totalSpace;
		rect<s32> pos = isHorizontal?rect<s32>(scrollPos+rd<f32,s32>(currentPos), 0, scrollPos+rd<f32,s32>(currentPos+delta), totalOtherSpace):rect<s32>(0, scrollPos+rd<f32,s32>(currentPos), totalOtherSpace, scrollPos+rd<f32,s32>(currentPos+delta));
		if(ele->mustMaintainAspectRatio()){
			pos = makeXic(pos, ele->getAspectRatio());
		}
		ele->setRelativePosition(pos);
		ele->updateAbsolutePosition();
		currentPos += delta;
	}
	maxScrollPosOut = Max(0, rd<f32,s32>(currentPos-totalSpace));
}

void AggregateGUIElement::updateAbsolutePosition(){
	absPosDirty = false;
	recalculateAbsolutePosition(false);//inefficient, because of multiple position calculation: IAggregatableGUIElement::updateAbsolutePosition();
	updateSubElementsPosition(subElements, maxScrollPos);
	updateSubElementsPosition(activatedSubElements, maxScrollPosActive);
}

void AggregateGUIElement::clear(){
	for(uint32_t i=0; i<subElements.size(); i++){
		subElements[i]->remove();
	}
	for(uint32_t i=0; i<activatedSubElements.size(); i++){
		activatedSubElements[i]->remove();
	}
	subElements.clear();
	activatedSubElements.clear();
	absPosDirty = true;//updateAbsolutePosition();
}

void AggregateGUIElement::addSubElement(IAggregatableGUIElement* subElement){
	subElements.push_back(subElement);
	addChild(subElement);
	absPosDirty = true;//updateAbsolutePosition();
}

void AggregateGUIElement::removeSubElement(irr::u32 i){
	IAggregatableGUIElement* ele = subElements[i];
	subElements.erase(subElements.begin()+i);
	ele->remove();
	absPosDirty = true;//updateAbsolutePosition();
}

void AggregateGUIElement::addActivatedSubElement(IAggregatableGUIElement* subElement){
	activatedSubElements.push_back(subElement);
	addChild(subElement);
	absPosDirty = true;//updateAbsolutePosition();
}

void AggregateGUIElement::removeActivatedSubElement(irr::u32 i){
	IAggregatableGUIElement* ele = activatedSubElements[i];
	activatedSubElements.erase(activatedSubElements.begin()+i);
	ele->remove();
	absPosDirty = true;//updateAbsolutePosition();
}

irr::u32 AggregateGUIElement::getActivatedSubElementCount() const{
	return activatedSubElements.size();
}

irr::u32 AggregateGUIElement::getSubElementCount() const{
	return subElements.size();
}

IAggregatableGUIElement* AggregateGUIElement::getSubElement(irr::u32 i){
	return subElements[i];
}

IAggregatableGUIElement* AggregateGUIElement::getActivatedSubElement(irr::u32 i){
	return activatedSubElements[i];
}

static const double minScrollDeccelerationUpdateDeltaTime = 0.033;

static const double scrollDecelleration = 1.0; // in parts per second (1 part = scrollspeed on release)
static const double scrollExponent = 3.0;

void AggregateGUIElement::updateScrollSpeed(){
	double time = getSecs();
	double dt = time-lastTime;
	if(dt>minScrollDeccelerationUpdateDeltaTime){
		if(scrollState==2){//decellerating
			double currentScrollSpeed = scrollSpeed*Max(0.0, pow(1.0-scrollDecelleration*(time-releaseTime), scrollExponent));
			s32 lowerBound = -Max(maxScrollPos, maxScrollPosActive);
			scrollPos = Clamp(rd<double,s32>(scrollPos+currentScrollSpeed*dt), lowerBound, 0);
			if(scrollbar!=NULL){scrollbar->setPos(lowerBound!=0?((double)scrollPos)/((double)lowerBound):0.0, false);}
			absPosDirty = true;//updateAbsolutePosition();
			if(fabs(currentScrollSpeed)<stopScrollSpeed || scrollPos==0 || scrollPos==lowerBound){
				scrollState = 0;
			}
		}else{
			scrollSpeed = ((double)(scrollPos-lastScrollPos))/(time-lastTime);
		}
		lastScrollPos = scrollPos;
		lastTime = time;
	}
}

bool AggregateGUIElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		if(m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			if(scrollState==2){setActivationLock(true);}
			scrollState = isScrollable?1:0;
			storedMousePos = vector2d<s32>(m.X, m.Y);
			storedScrollPos = scrollPos;
			scrollSpeed = 0.0;
			lastScrollPos = scrollPos;
			lastTime = getSecs();
			isMouseDown = true;
		}else if(m.Event==EMIE_LMOUSE_LEFT_UP){
			bool res = IAggregatableGUIElement::OnEvent(event);//process parents before unlocking
			if(scrollState>0){setActivationLock(false);}
			scrollState = 2;
			releaseTime = getSecs();//TODO: new decelleration
			isMouseDown = false;
			return res;
		}else if(m.Event==EMIE_MOUSE_MOVED){
			if(scrollState==1){
				s32 delta = isHorizontal?(m.X-storedMousePos.X):(m.Y-storedMousePos.Y);
				s32 totalSpace = isHorizontal?AbsoluteRect.getWidth():AbsoluteRect.getHeight();
				s32 lowerBound = -Max(maxScrollPos, maxScrollPosActive);
				scrollPos = Clamp(storedScrollPos+delta, lowerBound, 0);
				if(scrollbar!=NULL){scrollbar->setPos(lowerBound!=0?((double)scrollPos)/((double)lowerBound):0.0, false);}
				if(abs(delta)>ACTIVATE_CLICK_THRESHOLD*totalSpace){setActivationLock(true);}//set activation lock if scrolling to avoid activation of children
				absPosDirty = true;
			}
		}else if(m.Event==EMIE_MOUSE_WHEEL && isScrollable){
			setScrollPosition(Clamp(getScrollPosition()-m.Wheel*getScrollStepSize(), 0.f, 1.f), true);
			return true;
		}
	}else if(event.EventType==EET_GUI_EVENT){
		const SEvent::SGUIEvent& g = event.GUIEvent;
		if(g.EventType==EGET_COUNT){//Deactivate siblings of activated element
			bool isActiveChild = false;
			for(uint32_t i=0; i<subElements.size() && !isActiveChild; i++){
				isActiveChild = subElements[i]==g.Caller && subElements[i]->isActive();
			}
			for(uint32_t i=0; i<activatedSubElements.size() && !isActiveChild; i++){
				isActiveChild = activatedSubElements[i]==g.Caller && activatedSubElements[i]->isActive();
			}
			if(isActiveChild && !multiSelect){
				for(uint32_t i=0; i<subElements.size(); i++){
					if(subElements[i]!=g.Caller){subElements[i]->setActive(false, false);}
				}
				for(uint32_t i=0; i<activatedSubElements.size(); i++){
					if(activatedSubElements[i]!=g.Caller){activatedSubElements[i]->setActive(false, false);}
				}
			}
			irr::SEvent event;
			event.EventType = EET_GUI_EVENT;
			event.GUIEvent = SEvent::SGUIEvent{this, g.Caller, EGET_LISTBOX_CHANGED};
			Parent->OnEvent(event);
			return true;
		}
	}
	return IAggregatableGUIElement::OnEvent(event);
}

irr::f32 AggregateGUIElement::getScrollStepSize() const{
	s32 totalSpace = isHorizontal?AbsoluteRect.getWidth():AbsoluteRect.getHeight();
	s32 trueMaxScrollPos = Max(maxScrollPos, maxScrollPosActive);
	if(trueMaxScrollPos>0){
		f32 scrollStep = SCROLL_WHEEL_PART*((f32)totalSpace)/((f32)trueMaxScrollPos);
		scrollStep = 1.f/ceilf(1.f/scrollStep);//make sure 1 = n * scrollStep with n = natural number
		scrollStep += 1.f/(f32)trueMaxScrollPos;//compensate for rounding errors
		return scrollStep;
	}
	return 0.f;
}

void AggregateGUIElement::draw(){
	if(isVisible()){
		if(absPosDirty){updateAbsolutePosition();}
		updateScrollSpeed();
		IGUISkin* skin = Environment->getSkin();
		if(isExtendableSkin(skin)){
			((AggregateSkinExtension*)((IExtendableSkin*)skin)->getExtension(aggregatableSkinExtensionName, getID()))->drawSunkenPane(this, AbsoluteRect, &AbsoluteClippingRect);
		}
		IAggregatableGUIElement::draw();//draw children etc
	}
}

void AggregateGUIElement::setActivationLock(bool on){
	for(uint32_t i=0; i<subElements.size(); i++){
		subElements[i]->setActivationLock(on);
	}
	for(uint32_t i=0; i<activatedSubElements.size(); i++){
		activatedSubElements[i]->setActivationLock(on);
	}
	IAggregatableGUIElement::setActivationLock(on);
}

void AggregateGUIElement::setMultiSelectable(bool multiSelect){
	this->multiSelect = multiSelect;
}

bool AggregateGUIElement::getMultiSelectable(){
	return multiSelect;
}

void AggregateGUIElement::OnScrollBarChanged(ScrollBar* bar){
	if(scrollbar){
		setScrollPosition(scrollbar->getPos(), false);
	}
}

void AggregateGUIElement::setScrollPosition(irr::f32 pos, bool notifyScrollbar){
	s32 lowerBound = -Max(maxScrollPos, maxScrollPosActive);
	scrollPos = rd<f32,s32>(pos*lowerBound);
	scrollSpeed = 0.0;
	scrollState = 0;
	absPosDirty = true;
	if(notifyScrollbar && scrollbar){
		scrollbar->setPos(pos, false);
	}
}

irr::f32 AggregateGUIElement::getScrollPosition() const{
	f32 lowerBound = -Max(maxScrollPos, maxScrollPosActive);
	return ((f32)scrollPos)/lowerBound;
}

void AggregateGUIElement::setActive(bool active){
	IAggregatableGUIElement::setActive(active);
	if(!activatedSubElements.empty()){
		for(uint32_t i=0; i<subElements.size(); i++){
			subElements[i]->setVisible(!active);
		}
		for(uint32_t i=0; i<activatedSubElements.size(); i++){
			activatedSubElements[i]->setVisible(active);
		}
	}
	absPosDirty = true;
	//updateSubElementsPosition(subElements, maxScrollPos);
	//updateSubElementsPosition(activatedSubElements, maxScrollPosActive);
}

int32_t AggregateGUIElement::getSingleSelected(IAggregatableGUIElement** outGUIElement){
	for(u32 i=0; i<getSubElementCount(); i++){
		IAggregatableGUIElement* ele = getSubElement(i);
		if(ele->isActive()){
			if(outGUIElement){*outGUIElement = ele;}
			return i;
		}
	}
	return -1;
}
