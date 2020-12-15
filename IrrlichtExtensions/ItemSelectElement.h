#ifndef ItemSelectElement_H_INCLUDED
#define ItemSelectElement_H_INCLUDED

#include "IItemOrganizer.h"
#include "ForwardDeclarations.h"

#include <IGUIElement.h>
#include <IEventReceiver.h>

#include <map>
#include <list>
#include <regex>
#include <functional>

class AggregateGUIElement;
class BeautifulGUIButton;
class IAggregatableGUIElement;
class ItemSelectEditBoxCallback;
class Drawer2D;
class ItemSelectElement;
class CMBox;

//! Interface for retrieving icons to depict the items
class IItemSelectIconSource{

	protected:
	
	irr::video::ITexture* ascending;
	irr::video::ITexture* descending;
	irr::video::ITexture* mkdir;
	irr::video::ITexture* file;
	irr::video::ITexture* folder;
	
	public:
	
	IItemSelectIconSource(irr::video::ITexture* ascending, irr::video::ITexture* descending, irr::video::ITexture* mkdir, irr::video::ITexture* file, irr::video::ITexture* folder);
	
	virtual ~IItemSelectIconSource(){}
	
	virtual irr::video::ITexture* getAscendingIcon();
	
	virtual irr::video::ITexture* getDescendingIcon();
	
	virtual irr::video::ITexture* getMkdirIcon();
	
	virtual irr::video::ITexture* getFolderIcon();
	
	virtual irr::video::ITexture* getItemIcon(const IItemOrganizer::Item& item);
	
};

//! Interface for a result receiving callback
class IItemSelectCallback{
	
	public:
	
	enum Action{
		SELECT,//! when an item has been selected without closing the dialog
		DESELECT,//! when an item has been deselected without closing the dialog
		SAVE,//! when save has been pressed
		OPEN,//! when open has been pressed
		CANCEL,//! when cancel has been pressed
		ACTION_COUNT
	};
	
	virtual ~IItemSelectCallback(){}
	
	//! item may be NULL in case of CANCEL or DESELECT or SAVE, absolutePath empty if not applicable
	virtual void OnItemSelect(Action action, IItemOrganizer::Item* item, const std::string& absolutePath, ItemSelectElement* ele, IItemOrganizer* organizer) = 0;
	
};

//! Dialog for selecting Items from an IItemOrganizer, asserts the IGUIFont can safely be casted to FlexibleFont
class ItemSelectElement : public irr::gui::IGUIElement{
	friend class ItemSelectEditBoxCallback;
	
	private:
	
	irr::IrrlichtDevice* device;
	Drawer2D* drawer;
	
	IItemOrganizer* organizer;
	IItemSelectCallback* itemcbk;
	
	IItemSelectIconSource* source;
	
	bool isSaveDialog;
	
	irr::gui::IGUIEnvironment* env;
	irr::video::IVideoDriver* driver;
	
	const ILanguagePhrases* lang;
	
	irr::gui::IGUIEditBox* nameEdit;
	
	AggregateGUIElement* placesAgg;
	
	AggregateGUIElement* pathAgg;
	std::map<irr::gui::IGUIElement*, std::string> button2path;
	BeautifulGUIButton* mkdirBut;
	
	AggregateGUIElement* filesAndHeaderAgg;
	AggregateGUIElement* filesAgg;
	std::vector<BeautifulGUIButton*> fieldButtons;
	uint32_t sortIndex;
	bool sortAscending;
	std::vector<IItemOrganizer::Item*> items;
	int32_t selectedIndex;//index of items
	std::vector<uint32_t> lineIndex2ItemIndex;
	
	BeautifulGUIButton* positive;
	BeautifulGUIButton* negative;
	
	irr::gui::IGUIElement* parent;//parent for children gui (either this or an intermediate window)
	irr::core::rect<irr::s32> navBarRect;
	irr::core::rect<irr::s32> fileListRect;
	irr::s32 aggregationID, listElementAggregationID, invisibleAggregationID;
	irr::s32 buttonHeight;
	
	ItemSelectEditBoxCallback* cbk;
	
	CMBox* overwritebox;
	
	std::string originalPath;//current path when the dialog has been created
	
	std::regex regex;
	
	std::list<irr::gui::IGUIElement*> toRemove;//workaround for problem with remove during event processing
	
	void createPlacesGUI(const irr::core::rect<irr::s32>& r, irr::s32 padding, irr::gui::IGUIElement* parent, irr::s32 buttonHeight);
	
	void createNavigationBar(bool createMkdirButton = false, irr::s32 mkdirButtonID = -1);
	
	void createFileList();
	
	public:
	
	//! create a open or save dialog with window, subElement are inserted in the horizontal aggregation next to save/open and close buttons if empty an empy element is inserted
	//! defaultPath is either a path with file (1) or without file but with a trailing delimeter (2). (1) is useful for default in save dialogs and (2) is only used for cd
	//! only items which mach the regex and directories are displayed
	ItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, const std::string& defaultPath, IItemOrganizer* organizer, IItemSelectCallback* itemcbk, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 invisibleAggregationID, irr::s32 mkdirButtonID, bool isSaveDialog, IItemSelectIconSource* source, const std::initializer_list<IAggregatableGUIElement*>& subElements = {}, irr::f32 w = .75f, irr::f32 h = .75f, const ILanguagePhrases* phrases = NULL, bool modal = true, const std::regex& regex = std::regex(".*"));
	
	//! create an open dialog inside a rectangle, placesPart and pathPart define the part of the height of the rectange where the places and the path gui will be created
	//! only items which mach the regex and directories are displayed
	ItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, IItemOrganizer* organizer, IItemSelectCallback* itemcbk, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 invisibleAggregationID, IItemSelectIconSource* source, const irr::core::rect<irr::s32>& rectangle, irr::f32 placesPart=.3f, irr::f32 pathPart=.1f, const ILanguagePhrases* phrases = NULL, irr::gui::IGUIElement* parent = NULL, const std::regex& regex = std::regex(".*"));
	
	virtual ~ItemSelectElement();
	
	virtual bool OnEvent(const irr::SEvent& event);
	
	virtual void cd(const std::string& path, bool resetPlacesSelection = true);
	
	virtual void clearSelection();
	
	virtual void OnPostRender(irr::u32 timeMs);
	
	virtual AggregateGUIElement* getFilesAggregation() const;
	
};

typedef std::function<void(IItemSelectCallback::Action action, IItemOrganizer::Item* item, const std::string& absolutePath, ItemSelectElement* ele, IItemOrganizer* organizer)> ItemSelectCallbackFunction;

//! an easy wrapper function to simplify usage of the ItemSelectElement in window mode for file systems, all created helper objects are automatically deleted when the ItemSelectElement window closes
ItemSelectElement* createItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, const std::string& defaultPath, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 invisibleAggregationID, irr::s32 mkdirButtonID, bool isSaveDialog, const ItemSelectCallbackFunction& OnItemSelect, IItemSelectIconSource* source, irr::f32 w = .75f, irr::f32 h = .75f, const ILanguagePhrases* phrases = NULL, bool modal = true, const std::regex& regex = std::regex(".*"));

#endif
