#include "AggregateSkinExtension.h"
#include "IExtendableSkin.h"

#include <iostream>

using namespace irr;
using namespace gui;

AggregateSkinExtension::AggregateSkinExtension(IExtendableSkin* skin, bool highlightIfActive, bool shallDrawSunkenPane, irr::video::SColor* bgColor):
	IAggregatableSkinExtension(skin, highlightIfActive){
	this->shallDrawSunkenPane = shallDrawSunkenPane;
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
		skin->draw3DSunkenPane(ele, skin->getColor(EGDC_3D_HIGH_LIGHT), false, false, rect, clip);
	}
}
