#ifndef AMLGUIElement_H_INCLUDED
#define AMLGUIElement_H_INCLUDED

#include <AggregateGUIElement.h>
#include <ForwardDeclarations.h>

#include <string>
#include <list>
#include <set>

class Drawer2D;
class ICommonAppContext;
class IScrollable;
class ScrollBar;

//! Interface for a receiver of AML actions (see link action attribute)
class IAMLActionCallback{

	public:
	
	virtual ~IAMLActionCallback(){}
	
	//! called if a user presses on a link with an action attribute
	virtual void OnAMLAction(const std::string& action) = 0;

};

//! AML = App Markup Language
//! Asserts the font of the skin is a FlexibleFont and can safely be casted to FlexibleFont.
class AMLGUIElement : public AggregateGUIElement{
	friend class AMLParseCallback;

	private:
	
	std::string language;
	
	bool printParsingErrors;
	std::string utf8Code;
	
	AggregateGUIElement* navGUI;
	irr::gui::IGUIButton* back;
	irr::gui::IGUIButton* fwd;
	irr::gui::IGUIButton* reset;
	irr::gui::IGUIButton* zoomOut;
	irr::gui::IGUIButton* zoomIn;
	irr::f32 buttonSpace;
	
	struct ButtonParams{
		std::string href;
		std::string action;
	};
	
	std::map<irr::gui::IGUIElement*, ButtonParams> button2Params;
	
	IAMLActionCallback* cbk;
	
	IAggregatableGUIElement* content;
	irr::s32 invisibleAggregationId;
	irr::s32 scrollbarId;
	
	irr::s32 contentIndex;
	
	std::string searchPath;
	
	ICommonAppContext* c;
	Drawer2D* drawer;
	
	std::list<std::string> backStack;
	std::list<std::string> fwdStack;
	std::string current;
	
	irr::f32 zoom;
	
	struct ScrollLink{
	
		ScrollLink();
	
		IScrollable* scrollable;
		ScrollBar* scrollbar;
	};
	
	std::map<std::string, ScrollLink> id2ScrollLink;
	
	std::set<irr::video::ITexture*> textures;
	
	void changeZoom(irr::f32 newZoom);
	
	bool contentSet;

	public:
	
	struct NavButtons{
		std::wstring backButtonLabel;
		irr::s32 backButtonId;
		std::wstring fwdButtonLabel;
		irr::s32 fwdButtonId;
		std::wstring resetButtonLabel;
		irr::s32 resetButtonId;
		std::wstring zoomOutButtonLabel;
		irr::s32 zoomOutButtonId;
		std::wstring zoomInButtonLabel;
		irr::s32 zoomInButtonId;
		irr::f32 buttonSpace;//vertical space for the buttons
		irr::f32 buttonBarId;//useful for different styles, -1 for same style as AMLGUIElement
	};
	
	static const NavButtons defaultNavButtons;
	
	//! creates an empty AMLGUIElement
	//! Navigation Buttons wont't be created if NULL
	//! language: only parts which match the defined language or match "" are visible
	AMLGUIElement(ICommonAppContext* context, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, irr::s32 id, irr::s32 invisibleAggregationId, irr::s32 scrollbarId, std::string searchPath, std::string language = "", const NavButtons* navButtons = &defaultNavButtons, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	~AMLGUIElement();
	
	//! set a callback or NULL
	void setActionCallback(IAMLActionCallback* cbk = NULL);
	
	//! returns true if successful
	bool setCode(const std::string& utf8Code, bool printParsingErrors = true);
	
	//! adaptPath: if true changes the search path to the path of the given file
	//! returns true if successful
	bool setCodeFromFile(const std::string& path, bool printParsingErrors = true, bool adaptPath = true);
	
	void setLanguage(const std::string& language);
	
	bool OnEvent(const irr::SEvent& event);
	
	//! parses and (re)creates the GUI
	bool parseAndCreateGUI();
	
	//! return to the last page
	void goBack();
	
	//! go to the next (previously left) page
	void goForward();
	
	//! go to file while using the back and forward stack
	void gotoFile(const std::string& path);
	
	//! returns true if it has some content (AML code set by any applicable method)
	bool hasContent() const;
	
};

#endif
