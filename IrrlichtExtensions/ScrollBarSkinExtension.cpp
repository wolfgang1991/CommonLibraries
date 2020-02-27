#include "ScrollBarSkinExtension.h"

#include <IVideoDriver.h>
#include <IrrlichtDevice.h>
#include <Drawer2D.h>

#include <algorithm>

using namespace irr;
using namespace gui;
using namespace video;
using namespace core;

ScrollBarSkinExtension::ScrollBarSkinExtension(IExtendableSkin* skin, const std::initializer_list<irr::video::SColor>& colors, irr::f32 recommendedHandleSize, irr::f32 maxHandleSize):
	IAggregatableSkinExtension(skin, false),
	recommendedHandleSize(recommendedHandleSize),
	maxHandleSize(maxHandleSize){
	std::copy(colors.begin(), colors.end(), this->colors);
	this->device = skin->getDevice();
	driver = device->getVideoDriver();
}

ScrollBarSkinExtension::~ScrollBarSkinExtension(){
}

irr::IrrlichtDevice* ScrollBarSkinExtension::getDevice(){
	return device;
}

void ScrollBarSkinExtension::drawScrollbar(IGUIElement* ele, const irr::core::rect<irr::s32>& handleRect, const irr::core::rect<irr::s32>& scrollRect, const irr::core::rect<irr::s32>* clip){
	driver->draw2DRectangle(colors[SCROLL_BACKGROUND], scrollRect, clip);
	driver->draw2DRectangle(colors[SCROLL_HANDLE], handleRect, clip);
}

ScrollBarImageSkinExtension::ScrollBarImageSkinExtension(IExtendableSkin* skin, const std::initializer_list<irr::video::SColor>& colors, irr::f32 recommendedHandleSize, irr::f32 maxHandleSize, Drawer2D* drawer, irr::f32 uvCornerSize, irr::f32 realCornerSize, irr::f32 handleOverSize, irr::video::ITexture* handleImg, irr::video::ITexture* scrollImg):
	ScrollBarSkinExtension(skin, colors, recommendedHandleSize, maxHandleSize),
	drawer(drawer),
	handleImg(handleImg),
	scrollImg(scrollImg),
	uvCornerSize(uvCornerSize),
	realCornerSize(realCornerSize),
	handleOverSize(handleOverSize){}
	
void ScrollBarImageSkinExtension::drawScrollbar(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& handleRect, const irr::core::rect<irr::s32>& scrollRect, const irr::core::rect<irr::s32>* clip){
	if(handleImg==NULL || scrollImg==NULL){
		ScrollBarSkinExtension::drawScrollbar(ele, handleRect, scrollRect, clip);
	}else{
		drawer->drawRectWithCorner(rect<f32>(scrollRect.UpperLeftCorner.X+realCornerSize, scrollRect.UpperLeftCorner.Y+realCornerSize, scrollRect.LowerRightCorner.X-realCornerSize, scrollRect.LowerRightCorner.Y-realCornerSize), uvCornerSize, realCornerSize, scrollImg, colors[SCROLL_BACKGROUND], clip);
		if(handleOverSize>0.001){//eps
			dimension2d<s32> hDim = handleRect.getSize();
			rect<s32> newHandleRect(handleRect.UpperLeftCorner.X-handleOverSize*hDim.Width, handleRect.UpperLeftCorner.Y, handleRect.LowerRightCorner.X+handleOverSize*hDim.Width, handleRect.LowerRightCorner.Y);
			drawer->drawRectWithCorner(rect<f32>(newHandleRect.UpperLeftCorner.X+realCornerSize, newHandleRect.UpperLeftCorner.Y+realCornerSize, newHandleRect.LowerRightCorner.X-realCornerSize, newHandleRect.LowerRightCorner.Y-realCornerSize), uvCornerSize, realCornerSize, handleImg, colors[SCROLL_HANDLE], NULL);
		}else{
			drawer->drawRectWithCorner(rect<f32>(handleRect.UpperLeftCorner.X+realCornerSize, handleRect.UpperLeftCorner.Y+realCornerSize, handleRect.LowerRightCorner.X-realCornerSize, handleRect.LowerRightCorner.Y-realCornerSize), uvCornerSize, realCornerSize, handleImg, colors[SCROLL_HANDLE], clip);
		}
	}
}
