#ifndef AMLBox_H_INCLUDED
#define AMLBox_H_INCLUDED

class ICommonAppContext;

#include "AMLGUIElement.h"
#include "ForwardDeclarations.h"

//! A messagebox with a single button displaying an AML file
class AMLBox : public irr::gui::IGUIElement{

	protected:
	
	void OnNegative();
	
	void OnPositive();
	
	irr::core::dimension2d<irr::u32> screenSize;
	bool closeOnScreenResize;
	bool mustDelete;

	public:

	irr::gui::IGUIEnvironment* env;
	irr::video::IVideoDriver* driver;
	irr::gui::IGUIWindow* win;
	AMLGUIElement* aml;
	irr::gui::IGUIButton* ok;

	int w, h;

	AMLBox(ICommonAppContext* c, irr::s32 aggregationStyleID, irr::s32 invisibleAggregationStyleId, irr::s32 scrollbarStyleId, const AMLGUIElement::NavButtons* navButtons = &AMLGUIElement::defaultNavButtons, const std::string& language = "", irr::f32 maxW = 1.0, irr::f32 maxH = 1.0, const wchar_t* buttonText = L"OK", irr::s32 buttonId = -1, bool modal = true, irr::s32 navButtonId = -1, irr::video::SColor* overrideTextColor = NULL);

	//! returns true if successful
	bool setCode(const std::string& utf8Code, bool printParsingErrors = true);
	
	void gotoFile(const std::string& path);

	virtual ~AMLBox();

	bool OnEvent(const irr::SEvent& event);

	void OnPostRender(irr::u32 timeMs);
	
	void setCloseOnScreenResize(bool closeOnScreenResize);
	
	irr::gui::IGUIButton* getButton() const;

};


#endif
