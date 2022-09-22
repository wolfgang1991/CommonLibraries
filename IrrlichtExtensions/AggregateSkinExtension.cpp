#include <AggregateSkinExtension.h>
#include <IExtendableSkin.h>
#include <Drawer2D.h>

#include <IrrlichtDevice.h>

#include <algorithm>
#include <iostream>

using namespace irr;
using namespace core;
using namespace gui;
using namespace video;

AggregateSkinExtension::AggregateSkinExtension(IExtendableSkin* skin, bool highlightIfActive, bool shallDrawSunkenPane, irr::video::ITexture* sunkenPaneBgAlpha, const irr::video::SColor* bgColor, bool highlightIfPressed):
	IAggregatableSkinExtension(skin, highlightIfActive, highlightIfPressed){
	this->shallDrawSunkenPane = shallDrawSunkenPane;
	dimension2d<u32> dim = skin->getDevice()->getVideoDriver()->getScreenSize();
	realCornerSize = 0.005*sqrt(dim.Width*dim.Height);
	this->sunkenPaneBgAlpha = sunkenPaneBgAlpha;
	useBgColor = bgColor!=NULL;
	if(useBgColor){
		this->bgColor = *bgColor;
	}
}
	
void AggregateSkinExtension::drawSunkenPane(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip){
	if(useBgColor){
		skin->draw2DRectangle(ele, bgColor, rect, clip);
	}
	if(shallDrawSunkenPane){
		if(sunkenPaneBgAlpha==NULL){
			skin->draw3DSunkenPane(ele, skin->getColor(EGDC_3D_HIGH_LIGHT), false, false, rect, clip);
		}else{
			core::rect<s32> newClip;
			if(clip){
				newClip = core::rect<s32>(
					std::max(clip->UpperLeftCorner.X, rect.UpperLeftCorner.X)-realCornerSize,
					std::max(clip->UpperLeftCorner.Y, rect.UpperLeftCorner.Y)-realCornerSize,
					std::min(clip->LowerRightCorner.X, rect.LowerRightCorner.X)+realCornerSize,
					std::min(clip->LowerRightCorner.Y, rect.LowerRightCorner.Y)+realCornerSize
				);
				clip = &newClip;
			}
			skin->getDrawer2D()->drawRectWithCorner(core::rect<f32>(rect.UpperLeftCorner.X, rect.UpperLeftCorner.Y, rect.LowerRightCorner.X, rect.LowerRightCorner.Y), 0.2f, realCornerSize, sunkenPaneBgAlpha, SColor(255,255,255,255), clip);
		}
	}
}
