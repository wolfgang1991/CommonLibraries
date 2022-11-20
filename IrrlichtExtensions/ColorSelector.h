#ifndef COLOR_SELECTOR_H_INCLUDED
#define COLOR_SELECTOR_H_INCLUDED

#include <ForwardDeclarations.h>
#include <ICommonAppContext.h>

#include <SColor.h>
#include <rect.h>

#include <list>

//TODO: refactor to inherit from IGUIElement and replace processEvent by OnEvent
class ColorSelector{

	public:

	ColorSelector(ICommonAppContext* context, irr::video::ITexture* alphaBack, irr::video::SColor Color, bool modal = true);

	~ColorSelector();

	//! returns last selected color
	irr::video::SColor getColor();

	void setColor(irr::video::SColor Color);

	//! make editor visible
	void select(bool SelectAlpha);

	//! should be called for each event
	void processEvent(irr::SEvent event);

	//! render "not gui" part
	void render();

	bool isVisible();

	//! true if pressed ok
	bool lastSuccess();

	private:
	ICommonAppContext* c;
	irr::video::SColor color;
	bool lastSucc;
	bool selectAlpha;

	irr::IrrlichtDevice* device;
	irr::video::IVideoDriver* driver;
	irr::gui::IGUIEnvironment* env;

	int w, h;
	irr::core::rect<irr::s32> wrect;
	irr::gui::IGUIWindow* win;
	irr::gui::IGUIStaticText* tr;
	irr::gui::IGUIStaticText* tg;
	irr::gui::IGUIStaticText* tb;
	irr::gui::IGUIStaticText* ta;
	irr::gui::IGUIEditBox* er;
	irr::gui::IGUIEditBox* eg;
	irr::gui::IGUIEditBox* eb;
	irr::gui::IGUIEditBox* ea;
	irr::gui::IGUIScrollBar* sr;
	irr::gui::IGUIScrollBar* sg;
	irr::gui::IGUIScrollBar* sb;
	irr::gui::IGUIScrollBar* sa;
	irr::gui::IGUIButton* bok;
	irr::gui::IGUIButton* bcancel;

	std::list<irr::gui::IGUIElement*> allowedFocus;

	irr::video::ITexture* alpha_back;

	void addRecursiveAllowedFocus(irr::gui::IGUIElement* ele);

};

#endif
