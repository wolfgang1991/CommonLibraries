#include <ProgressBar.h>
#include <mathUtils.h>
#include <Drawer2D.h>
#include <ProgressBarSkinExtension.h>

#include <timing.h>

#include <IGUIEnvironment.h>
#include <IGUIFont.h>

#include <sstream>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

ProgressBar::ProgressBar(Drawer2D* drawer, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, bool drawText, irr::s32 id, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, 1.f, recommendedSpace, 1.f, false, false, id, data, parent, rectangle),
	pos(0.f),
	drawText(drawText),
	sqrtArea(0.0),
	drawer(drawer){
	dimension2d<u32> dim = environment->getVideoDriver()->getScreenSize();
	sqrtArea = sqrt(dim.Width*dim.Height);
	overrideColorEnabled = false;
	overrideColor = SColor(255,0,0,0);
}

void ProgressBar::setPos(irr::f32 pos){
	this->pos = pos;
}

irr::f32 ProgressBar::getPos(){
	return pos;
}

void ProgressBar::draw(){
	if(isVisible()){
		IGUISkin* guiSkin = Environment->getSkin();
		if(isExtendableSkin(guiSkin)){
			ProgressBarSkinExtension* sext = (ProgressBarSkinExtension*)((IExtendableSkin*)guiSkin)->getExtension(aggregatableSkinExtensionName, getID());//must be a ScrollBar Skin otherwise the id has been set wrong
			std::wstringstream ss; ss << rd<f32,s32>(pos*100.f) << L"%";
			std::wstring s = ss.str();
			rect<s32> afterTextRect(AbsoluteRect);
			IGUIFont* font = guiSkin->getFont();
			if(drawText){
				afterTextRect.UpperLeftCorner.X += Min((s32)font->getDimension(s.c_str()).Width, afterTextRect.getWidth());
			}
			f32 realCornerSize = Min(afterTextRect.getWidth()*sext->uvCornerSize, afterTextRect.getHeight()*sext->uvCornerSize);
			s32 rrcs = rd<f32,s32>(realCornerSize);
			if(drawText){
				font->draw(s.c_str(), rect<s32>(AbsoluteRect.UpperLeftCorner, vector2d<s32>(afterTextRect.UpperLeftCorner.X+rrcs, afterTextRect.LowerRightCorner.Y)), overrideColorEnabled?overrideColor:guiSkin->getColor(isEnabled()?EGDC_BUTTON_TEXT:EGDC_GRAY_TEXT), true, true, &AbsoluteClippingRect);
			}
			rect<s32> smallerRect(afterTextRect.UpperLeftCorner.X+rrcs, afterTextRect.UpperLeftCorner.Y+rrcs, afterTextRect.LowerRightCorner.X-rrcs, afterTextRect.LowerRightCorner.Y-rrcs);
			f32 padding = Min(smallerRect.getWidth()*sext->padding, smallerRect.getHeight()*sext->padding);//TODO skin extension (auch für color) + lauflicht
			guiSkin->draw3DSunkenPane(this, SColor(255,0,0,0), true, false, smallerRect, &AbsoluteClippingRect);
			double colorProgress = 0.5+0.5*sin(2*3.14*sext->colorSwitchFrequency*getSecs());
			drawer->drawRectWithCorner(rect<f32>(smallerRect.UpperLeftCorner.X+padding, smallerRect.UpperLeftCorner.Y+padding, smallerRect.UpperLeftCorner.X+padding+pos*(smallerRect.LowerRightCorner.X-smallerRect.UpperLeftCorner.X-2.f*padding), smallerRect.LowerRightCorner.Y-padding), sext->uvCornerSize, realCornerSize, sext->progress, sext->color0.getInterpolated(sext->color1, colorProgress), &AbsoluteClippingRect);
			IAggregatableGUIElement::draw();//draw children etc
		}
	}
}

void ProgressBar::setOverrideColor(irr::video::SColor* color){
	overrideColorEnabled = color!=NULL;
	if(overrideColorEnabled){
		overrideColor = *color;
	}
}
