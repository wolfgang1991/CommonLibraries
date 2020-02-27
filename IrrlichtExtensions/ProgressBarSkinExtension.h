#ifndef ProgressBarSkinExtension_H_INCLUDED
#define ProgressBarSkinExtension_H_INCLUDED

#include "ForwardDeclarations.h"
#include "IExtendableSkin.h"
#include "IAggregatableGUIElement.h"

#include <SColor.h>

//! Skin extension for vhf style scroll bars
class ProgressBarSkinExtension : public IAggregatableSkinExtension{

	public:
	
	irr::f32 padding;
	irr::video::SColor color0, color1;
	irr::f32 colorSwitchFrequency;
	irr::f32 uvCornerSize;
	irr::video::ITexture* progress;
	
	public:
	
	ProgressBarSkinExtension(IExtendableSkin* skin, irr::video::ITexture* progress, irr::f32 padding, irr::video::SColor color0, irr::video::SColor color1, irr::f32 colorSwitchFrequency, irr::f32 uvCornerSize = 0.f):
		IAggregatableSkinExtension(skin, false),
		padding(padding),
		color0(color0),
		color1(color1),
		colorSwitchFrequency(colorSwitchFrequency),
		uvCornerSize(uvCornerSize),
		progress(progress){}
	
	virtual ~ProgressBarSkinExtension(){}

};

#endif
