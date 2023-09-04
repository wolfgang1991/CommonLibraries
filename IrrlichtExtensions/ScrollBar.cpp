#include "ScrollBar.h"
#include "AggregateGUIElement.h"
#include "utilities.h"
#include "mathUtils.h"
#include "ScrollBarSkinExtension.h"
#include "IExtendableSkin.h"

#include <IVideoDriver.h>
#include <IGUIEnvironment.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

ScrollBar::ScrollBar(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, bool isHorizontal, irr::s32 id, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle, bool hideIfNothingToScroll):
	IAggregatableGUIElement(environment, recommendedSpace, 1.f, recommendedSpace, 1.f, false, false, id, data, parent, rectangle),
	isHorizontal(isHorizontal),
	scrollable(NULL),
	pos(0.f),
	sqrtArea(0.0),
	handleSize(0),
	handleRect(0,0,0,0),
	scrollSpace(0),
	scrolling(false),
	storedMPos(0,0),
	storedPos(0),
	hideIfNothingToScroll(hideIfNothingToScroll),
	originalSpace(recommendedSpace),
	collapsed(false){
	dimension2d<u32> dim = environment->getVideoDriver()->getScreenSize();
	sqrtArea = sqrt(dim.Width*dim.Height);
	updateAbsolutePosition();
}

ScrollBar::~ScrollBar(){
	unlinkScrollable();
}

void ScrollBar::updateAbsolutePosition(){
	IAggregatableGUIElement::updateAbsolutePosition();
	IGUISkin* guiSkin = Environment->getSkin();
	if(isExtendableSkin(guiSkin)){
		ScrollBarSkinExtension* skin = (ScrollBarSkinExtension*)((IExtendableSkin*)guiSkin)->getExtension(aggregatableSkinExtensionName, getID());
		handleSize = rd<double,s32>(Min(skin->recommendedHandleSize*sqrtArea, (double)skin->maxHandleSize*(isHorizontal?AbsoluteRect.getWidth():AbsoluteRect.getHeight())));
	}
	scrollSpace = isHorizontal?(AbsoluteRect.LowerRightCorner.X-AbsoluteRect.UpperLeftCorner.X-handleSize):(AbsoluteRect.LowerRightCorner.Y-AbsoluteRect.UpperLeftCorner.Y-handleSize);
}

bool ScrollBar::OnEvent(const irr::SEvent& event){
	if(hideIfNothingToScroll && scrollable!=NULL && !scrollable->isTrulyScrollable()){return IAggregatableGUIElement::OnEvent(event);}
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const irr::SEvent::SMouseInput& m = event.MouseInput;
		vector2d<s32> mPos(m.X, m.Y);
		if(m.Event==EMIE_LMOUSE_PRESSED_DOWN){
			scrolling = handleRect.isPointInside(mPos);
			storedMPos = mPos;
			storedPos = pos;
		}else if(m.Event==EMIE_MOUSE_MOVED && scrolling){
			pos = Clamp(storedPos + (double)(isHorizontal?(mPos.X-storedMPos.X):(mPos.Y-storedMPos.Y))/(double)scrollSpace, 0.0, 1.0);
			if(scrollable!=NULL){scrollable->OnScrollBarChanged(this);}
		}else if(m.Event==EMIE_LMOUSE_LEFT_UP){
			if(!scrolling && AbsoluteRect.isPointInside(mPos)){
				pos = Clamp(((double)(isHorizontal?(mPos.X-AbsoluteRect.UpperLeftCorner.X):(mPos.Y-AbsoluteRect.UpperLeftCorner.Y))-handleSize*.5)/(double)scrollSpace, 0.0, 1.0);
				if(scrollable!=NULL){scrollable->OnScrollBarChanged(this);}
			}
			scrolling = false;
		}else if(m.Event==EMIE_MOUSE_WHEEL){
			if(scrollable){
				setPos(Clamp(getPos()-m.Wheel*scrollable->getScrollStepSize(), 0.f, 1.f), true);
			}else{
				setPos(Clamp(getPos()-m.Wheel*SCROLL_WHEEL_PART, 0.f, 1.f), true);
			}
			return true;
		}
	}
	return IAggregatableGUIElement::OnEvent(event);
}

void ScrollBar::setCollapsed(bool collapsed){
	if(this->collapsed!=collapsed){
		this->collapsed = collapsed;
		if(collapsed){
			setRecommendedSpace(0.f);
		}else{
			setRecommendedSpace(originalSpace);
		}
		if(Parent){Parent->updateAbsolutePosition();}
	}
}

void ScrollBar::linkToScrollable(IScrollable* scrollable){
	if(this->scrollable!=scrollable){//avoid infinite recursions
		this->scrollable = scrollable;
		scrollable->linkToScrollBar(this);
	}
}

void ScrollBar::unlinkScrollable(){
	if(scrollable!=NULL){
		IScrollable* tmp = scrollable;
		scrollable = NULL;
		tmp->unlinkScrollBar();
	}
}

void ScrollBar::setPos(irr::f32 pos, bool notifyLinks){
	this->pos = pos;
	if(notifyLinks && scrollable!=NULL){
		scrollable->OnScrollBarChanged(this);
	}
}

irr::f32 ScrollBar::getPos(){
	return pos;
}

void ScrollBar::draw(){
	if(isVisible()){
		setCollapsed(hideIfNothingToScroll && scrollable!=NULL && !scrollable->isTrulyScrollable());
		if(collapsed){return;}
		IGUISkin* guiSkin = Environment->getSkin();
		if(isExtendableSkin(guiSkin)){
			ScrollBarSkinExtension* skin = (ScrollBarSkinExtension*)((IExtendableSkin*)guiSkin)->getExtension(aggregatableSkinExtensionName, getID());//must be a ScrollBar Skin otherwise the id has been set wrong
			s32 delta = rd<f32,s32>(pos*scrollSpace);
			if(isHorizontal){
				handleRect = rect<s32>(AbsoluteRect.UpperLeftCorner.X+delta, AbsoluteRect.UpperLeftCorner.Y, AbsoluteRect.UpperLeftCorner.X+delta+handleSize, AbsoluteRect.LowerRightCorner.Y);
			}else{
				handleRect = rect<s32>(AbsoluteRect.UpperLeftCorner.X, AbsoluteRect.UpperLeftCorner.Y+delta, AbsoluteRect.LowerRightCorner.X, AbsoluteRect.UpperLeftCorner.Y+delta+handleSize);
			}
			skin->drawScrollbar(this, handleRect, AbsoluteRect, &AbsoluteClippingRect);
		}
		IAggregatableGUIElement::draw();//draw children etc
	}
}
