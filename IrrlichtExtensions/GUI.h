#ifndef GUI_H_INCLUDED
#define GUI_H_INCLUDED

class IniFile;

namespace irr{
	namespace gui{
		class IGUIWindow;
		class IGUIElement;
	}
}

#include <list>
#include <string>
#include <map>

irr::gui::IGUIWindow* addWindowForRatio(irr::gui::IGUIEnvironment* env, IniFile* gini2, irr::core::rect<irr::s32> parentRect, bool modal = false, irr::gui::IGUIElement* parent = NULL, irr::s32 id = -1);

//! Das größtmögliche Rectangle innerhalb des angegebenen mit dem Seitenverhältnis ratio liefern
irr::core::rect<irr::s32> calcRatioRect(irr::core::rect<irr::s32> original, double ratio);

class ExtendedGUIElement{
	
	public:

	ExtendedGUIElement(irr::gui::IGUIElement* ele, int visDomain, const std::string& defaults);

	virtual ~ExtendedGUIElement();

	virtual void setDefaults(){}

	virtual void OnEvent(const irr::SEvent::SGUIEvent& event){}

	// wird am Ende des Einlesens aufgerufen
	virtual void OnFinish(std::map<std::string, ExtendedGUIElement*>* elements){}

	virtual void sendIntMessage(ExtendedGUIElement* caller, int message){}

	irr::gui::IGUIElement* ele;
	int visDomain;
	std::string defaults;

};

class ExtendedEditBox : public ExtendedGUIElement{
	
	public:

	ExtendedEditBox(irr::gui::IGUIElement* ele, int visDomain, const std::string& defaults);

	void setDefaults();

};

class ExtendedCheckBox : public ExtendedGUIElement{

	public:

	//! Hides und Sitches geben Visibility Domains an
	ExtendedCheckBox(irr::gui::IGUIElement* ele, int visDomain, const std::string& defaults, int hides, int switches, const std::list<std::string>& exclusiveCounterParts);

	void setDefaults();

	void OnEvent(const irr::SEvent::SGUIEvent& event);

	void OnFinish(std::map<std::string, ExtendedGUIElement*>* elements);

	void sendIntMessage(ExtendedGUIElement* caller, int message);

	private:

	int hides, switches;
	std::list<std::string> exclusiveCounterParts;
	std::list<ExtendedGUIElement*> hideElements, switchElements, exclusiveElements;

};

//! Interface for maps of unique ids to gui elements or rectangles
class IGUIDefinition{
	
	public:
	
	virtual ~IGUIDefinition(){}
	
	//! return NULL if no available
	virtual irr::gui::IGUIElement* getElement(const std::string& uid) = 0;
	
	//! returns empty rectangle (0,0,0,0) if not available
	virtual irr::core::rect<irr::s32> getSpecialRectangle(const std::string& uid) = 0;
	
};

//! Für Gui in Ini Dateien
class GUI : public IGUIDefinition{

	public:

	//! Elemente aus Ini laden und defaults setzen
	GUI(irr::gui::IGUIEnvironment* env, IniFile* ini, irr::core::rect<irr::s32> r, irr::gui::IGUIElement* parent, bool keep_aspect_ratio);

	~GUI();

	void Update(const irr::SEvent::SGUIEvent& event);

	irr::gui::IGUIElement* getElement(const std::string& uid);

	void setDefaults();

	//! rectangle des GUIElements oder der speziellen Definition
	irr::core::rect<irr::s32> getSpecialRectangle(const std::string& uid);

	const std::map<std::string, irr::core::rect<irr::s32> >& getAllSpecialRectangles();
	
	void setVisible(bool visible);
	
	bool isVisible() const;

	private:

	irr::gui::IGUIEnvironment* env;
	irr::core::rect<irr::s32> r; irr::gui::IGUIElement* parent; bool keep_aspect_ratio;

	std::map<std::string, ExtendedGUIElement*> elements;
	std::map<std::string, irr::core::rect<irr::s32> > rects;

};

#endif
