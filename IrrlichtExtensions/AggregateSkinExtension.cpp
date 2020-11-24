#include <AggregateSkinExtension.h>
#include <IExtendableSkin.h>
#include <Drawer2D.h>

#include <IrrlichtDevice.h>

#include <iostream>

using namespace irr;
using namespace gui;
using namespace core;
using namespace video;

AggregateSkinExtension::AggregateSkinExtension(IExtendableSkin* skin, bool highlightIfActive, bool shallDrawSunkenPane, irr::video::ITexture* sunkenPaneBgAlpha, irr::video::SColor* bgColor, bool highlightIfPressed):
	IAggregatableSkinExtension(skin, highlightIfActive, highlightIfPressed){
	this->shallDrawSunkenPane = shallDrawSunkenPane;
	dimension2d<u32> dim = skin->getDevice()->getVideoDriver()->getScreenSize();
	realCornerSize = 0.01*sqrt(dim.Width*dim.Height);
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
			skin->getDrawer2D()->drawRectWithCorner(core::rect<f32>(rect.UpperLeftCorner.X+realCornerSize, rect.UpperLeftCorner.Y+realCornerSize, rect.LowerRightCorner.X-realCornerSize, rect.LowerRightCorner.Y-realCornerSize), 0.29297f, realCornerSize, sunkenPaneBgAlpha, SColor(255,255,255,255), clip);
		}
	}
}
