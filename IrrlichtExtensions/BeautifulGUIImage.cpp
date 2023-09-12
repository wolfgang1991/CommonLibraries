#include "BeautifulGUIImage.h"
#include "Drawer2D.h"
#include "utilities.h"
#include "mathUtils.h"

#include <ITexture.h>
#include <IVideoDriver.h>
#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

static const irr::video::SColor white(255,255,255,255);

static f32 calcTexAspectRatio(ITexture* tex){
	return tex!=NULL?((irr::f32)tex->getOriginalSize().Width)/((irr::f32)tex->getOriginalSize().Height):1.f;
}

BeautifulGUIImage::BeautifulGUIImage(Drawer2D* drawer, irr::video::ITexture* tex, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, bool maintainAspectRatio, irr::s32 id, irr::video::SColor color, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, calcTexAspectRatio(tex), recommendedSpace, calcTexAspectRatio(tex), maintainAspectRatio, false, id, data, parent, rectangle),
	tex(tex),
	drawer(drawer),
	colors{color, color, color, color}{
	setName("BeautifulGUIImage");
}
	
void BeautifulGUIImage::draw(){
	if(isVisible() && tex!=NULL){
		IVideoDriver* driver = drawer->getDevice()->getVideoDriver();
		rect<s32> vp = driver->getViewPort();
		driver->setViewPort(AbsoluteClippingRect);
		drawer->setTextureWrap(ETC_CLAMP, ETC_CLAMP);
		drawer->setColors(colors);
		rect<s32> finalRect = maintainAspectRatio?makeXic(AbsoluteRect, aspectRatio):AbsoluteRect;
		drawer->draw(tex, rect<s32>(finalRect.UpperLeftCorner-AbsoluteClippingRect.UpperLeftCorner, finalRect.LowerRightCorner-AbsoluteClippingRect.UpperLeftCorner));
		drawer->setColor(white);
		drawer->setTextureWrap();
		driver->setViewPort(vp);
		IAggregatableGUIElement::draw();//draw children etc
	}
}

void BeautifulGUIImage::setColor(uint32_t i, irr::video::SColor color){
	colors[i] = color;
}

void BeautifulGUIImage::setColor(irr::video::SColor color){
	for(uint32_t i=0; i<4; i++){
		colors[i] = color;
	}
}

void BeautifulGUIImage::setTexture(irr::video::ITexture* tex){
	this->tex = tex;
	aspectRatio = activeAspectRatio = calcTexAspectRatio(tex);
	if(Parent){
		Parent->updateAbsolutePosition();
	}
}
