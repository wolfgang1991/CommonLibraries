#ifndef AggregateSkinExtension_H_INCLUDED
#define AggregateSkinExtension_H_INCLUDED

#include "IAggregatableGUIElement.h"

//! Extension for aggregate gui elements
class AggregateSkinExtension : public IAggregatableSkinExtension{

	protected:
	
	bool shallDrawSunkenPane;
	
	irr::f32 realCornerSize;
	
	irr::video::ITexture* sunkenPaneBgAlpha;

	irr::video::SColor bgColor;
	bool useBgColor;

	public:
	
	AggregateSkinExtension(IExtendableSkin* skin, bool highlightIfActive, bool shallDrawSunkenPane, irr::video::ITexture* sunkenPaneBgAlpha = NULL, irr::video::SColor* bgColor = NULL, bool highlightIfPressed = false);
	
	virtual void drawSunkenPane(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip);

};

#endif
