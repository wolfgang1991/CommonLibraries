#ifndef IExtendableSkin_H_INCLUDED
#define IExtendableSkin_H_INCLUDED

#include "ForwardDeclarations.h"

#include <IGUISkin.h>

#include <unordered_map>
#include <map>
#include <string>

class Drawer2D;

class ISkinExtension{

	public:
	
	virtual ~ISkinExtension(){}
	
	virtual const std::string& getName() = 0;

};

//! provides default implementation of required methods using a "parent" skin.
//! Also provides a possibility to register extensions for new gui elements
class IExtendableSkin : public irr::gui::IGUISkin{

	protected:
	
	Drawer2D* drawer;
	irr::IrrlichtDevice* device;
	irr::gui::IGUISkin* parent;
	
	std::unordered_map<std::string, std::map<uint32_t, ISkinExtension*> > extensions;//name -> (style id -> extension)
	
	public:
	
	IExtendableSkin(irr::gui::IGUISkin* parent, Drawer2D* drawer);
	
	virtual ~IExtendableSkin();
	
	//! adds a new extension and deletes the old one if one is there
	virtual void registerExtension(ISkinExtension* extension, uint32_t styleId = 0);
	
	//! returns NULL if the given style id or extension name is not available
	virtual ISkinExtension* getExtension(const std::string& extensionName, uint32_t styleId = 0);
	
	virtual irr::gui::IGUISkin* getParentSkin();
	
	virtual irr::IrrlichtDevice* getDevice();
	
	virtual Drawer2D* getDrawer2D();
	
	virtual irr::video::SColor getColor(irr::gui::EGUI_DEFAULT_COLOR color) const;

	virtual void setColor(irr::gui::EGUI_DEFAULT_COLOR which, irr::video::SColor newColor);

	virtual irr::s32 getSize(irr::gui::EGUI_DEFAULT_SIZE size) const;

	virtual const wchar_t* getDefaultText(irr::gui::EGUI_DEFAULT_TEXT text) const;

	virtual void setDefaultText(irr::gui::EGUI_DEFAULT_TEXT which, const wchar_t* newText);

	virtual void setSize(irr::gui::EGUI_DEFAULT_SIZE which, irr::s32 size);

	virtual irr::gui::IGUIFont* getFont(irr::gui::EGUI_DEFAULT_FONT which=irr::gui::EGDF_DEFAULT) const;

	virtual void setFont(irr::gui::IGUIFont* font, irr::gui::EGUI_DEFAULT_FONT which=irr::gui::EGDF_DEFAULT);

	virtual irr::gui::IGUISpriteBank* getSpriteBank() const;

	virtual void setSpriteBank(irr::gui::IGUISpriteBank* bank);

	virtual irr::u32 getIcon(irr::gui::EGUI_DEFAULT_ICON icon) const;

	virtual void setIcon(irr::gui::EGUI_DEFAULT_ICON icon, irr::u32 index);

	virtual void draw3DButtonPaneStandard(irr::gui::IGUIElement* element,
			const irr::core::rect<irr::s32>& rect,
			const irr::core::rect<irr::s32>* clip=0);

	virtual void draw3DButtonPanePressed(irr::gui::IGUIElement* element,
			const irr::core::rect<irr::s32>& rect,
			const irr::core::rect<irr::s32>* clip=0);

	virtual void draw3DSunkenPane(irr::gui::IGUIElement* element,
			irr::video::SColor bgcolor, bool flat, bool fillBackGround,
			const irr::core::rect<irr::s32>& rect,
			const irr::core::rect<irr::s32>* clip=0);

	virtual irr::core::rect<irr::s32> draw3DWindowBackground(irr::gui::IGUIElement* element,
			bool drawTitleBar, irr::video::SColor titleBarColor,
			const irr::core::rect<irr::s32>& rect,
			const irr::core::rect<irr::s32>* clip=0,
			irr::core::rect<irr::s32>* checkClientArea=0);

	virtual void draw3DMenuPane(irr::gui::IGUIElement* element,
			const irr::core::rect<irr::s32>& rect,
			const irr::core::rect<irr::s32>* clip=0);

	virtual void draw3DToolBar(irr::gui::IGUIElement* element,
			const irr::core::rect<irr::s32>& rect,
			const irr::core::rect<irr::s32>* clip=0);

	virtual void draw3DTabButton(irr::gui::IGUIElement* element, bool active,
			const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip=0, irr::gui::EGUI_ALIGNMENT alignment=irr::gui::EGUIA_UPPERLEFT);

	virtual void draw3DTabBody(irr::gui::IGUIElement* element, bool border, bool background,
			const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip=0, irr::s32 tabHeight=-1, irr::gui::EGUI_ALIGNMENT alignment=irr::gui::EGUIA_UPPERLEFT );

	virtual void drawIcon(irr::gui::IGUIElement* element, irr::gui::EGUI_DEFAULT_ICON icon,
			const irr::core::position2di position, irr::u32 starttime=0, irr::u32 currenttime=0,
			bool loop=false, const irr::core::rect<irr::s32>* clip=0);

	virtual void draw2DRectangle(irr::gui::IGUIElement* element, const irr::video::SColor &color,
			const irr::core::rect<irr::s32>& pos, const irr::core::rect<irr::s32>* clip);

	virtual irr::gui::EGUI_SKIN_TYPE getType() const;
	
};

bool isExtendableSkin(void* something);

#endif
