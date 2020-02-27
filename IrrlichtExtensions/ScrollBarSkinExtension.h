#ifndef ScrollBarSkinExtension_H_INCLUDED
#define ScrollBarSkinExtension_H_INCLUDED

#include "ForwardDeclarations.h"
#include "IExtendableSkin.h"
#include "IAggregatableGUIElement.h"

#include <SColor.h>
#include <rect.h>
#include <IGUIElement.h>

#include <map>

//! Skin extension for vhf style scroll bars
class ScrollBarSkinExtension : public IAggregatableSkinExtension{

	public:
	
	enum ScrollBarColorId{
		SCROLL_HANDLE,
		SCROLL_BACKGROUND,
		AGG_COLOR_COUNT
	};
	
	irr::video::SColor colors[AGG_COLOR_COUNT];
	irr::f32 recommendedHandleSize;//Part of whole screen (=sqrt(w*h))
	irr::f32 maxHandleSize;//Part of available space (available space = length of scroll bar)
	
	private:
	
	irr::IrrlichtDevice* device;
	irr::video::IVideoDriver* driver;
	
	public:
	
	ScrollBarSkinExtension(IExtendableSkin* skin, const std::initializer_list<irr::video::SColor>& colors, irr::f32 recommendedHandleSize, irr::f32 maxHandleSize);
	
	virtual ~ScrollBarSkinExtension();
	
	irr::IrrlichtDevice* getDevice();
	
	virtual void drawScrollbar(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& handleRect, const irr::core::rect<irr::s32>& scrollRect, const irr::core::rect<irr::s32>* clip=0);

};

class Drawer2D;

class ScrollBarImageSkinExtension : public ScrollBarSkinExtension{

	private:
	
	Drawer2D* drawer;
	irr::video::ITexture* handleImg;
	irr::video::ITexture* scrollImg;
	
	irr::f32 uvCornerSize;
	irr::f32 realCornerSize;
	irr::f32 handleOverSize;
	
	public:
	
	ScrollBarImageSkinExtension(IExtendableSkin* skin, const std::initializer_list<irr::video::SColor>& colors, irr::f32 recommendedHandleSize, irr::f32 maxHandleSize, Drawer2D* drawer, irr::f32 uvCornerSize, irr::f32 realCornerSize, irr::f32 handleOverSize = 0.f, irr::video::ITexture* handleImg = NULL, irr::video::ITexture* scrollImg = NULL);
	
	virtual void drawScrollbar(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& handleRect, const irr::core::rect<irr::s32>& scrollRect, const irr::core::rect<irr::s32>* clip=0);
	
};

#endif
