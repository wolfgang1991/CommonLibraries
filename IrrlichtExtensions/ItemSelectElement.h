#ifndef ItemSelectElement_H_INCLUDED
#define ItemSelectElement_H_INCLUDED

#include "IItemOrganizer.h"
#include "ForwardDeclarations.h"

#include <IGUIElement.h>
#include <IEventReceiver.h>

#include <map>

class AggregateGUIElement;
class BeautifulGUIButton;
class IAggregatableGUIElement;
class ItemSelectEditBoxCallback;
class IItemSelectIconSource;
class Drawer2D;

//! Dialog for selecting Items from an IItemOrganizer, asserts the IGUIFont can safely be casted to FlexibleFont
class ItemSelectElement : public irr::gui::IGUIElement{
	friend class ItemSelectEditBoxCallback;
	
	private:
	
	irr::IrrlichtDevice* device;
	Drawer2D* drawer;
	
	IItemOrganizer* organizer;
	
	IItemSelectIconSource* source;
	
	bool isSaveDialog;
	
	irr::gui::IGUIEnvironment* env;
	irr::video::IVideoDriver* driver;
	
	const ILanguagePhrases* lang;
	
	irr::gui::IGUIEditBox* nameEdit;
	
	AggregateGUIElement* pathAgg;
	std::map<irr::gui::IGUIElement*, std::string> button2path;
	irr::gui::IGUIButton* mkdirBut;
	
	AggregateGUIElement* filesAndHeaderAgg;
	AggregateGUIElement* filesAgg;
	std::vector<BeautifulGUIButton*> fieldButtons;
	uint32_t sortIndex;
	bool sortAscending;
	
	BeautifulGUIButton* positive;
	BeautifulGUIButton* negative;
	
	irr::gui::IGUIElement* parent;//parent for children gui (either this or an intermediate window)
	irr::core::rect<irr::s32> navBarRect;
	irr::core::rect<irr::s32> fileListRect;
	irr::s32 aggregationID, listElementAggregationID;
	irr::s32 buttonHeight;
	
	ItemSelectEditBoxCallback* cbk;
	
	void createPlacesGUI(const irr::core::rect<irr::s32>& r, irr::s32 padding, irr::gui::IGUIElement* parent, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 buttonHeight);
	
	void createNavigationBar(bool createMkdirButton = false, irr::s32 mkdirButtonID = -1);
	
	void createFileList();
	
	public:
	
	//! create a open or save dialog with window, subElement are inserted in the horizontal aggregation next to save/open and close buttons if empty an empy element is inserted
	ItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, const std::wstring& defaultSaveFileName, IItemOrganizer* organizer, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 mkdirButtonID, bool isSaveDialog, IItemSelectIconSource* source, const std::initializer_list<IAggregatableGUIElement*>& subElements = {}, irr::f32 w = .75f, irr::f32 h = .75f, ILanguagePhrases* phrases = NULL, bool modal = true);
	
	virtual ~ItemSelectElement();
	
	virtual void OnPostRender(irr::u32 timeMs);
	
	virtual bool OnEvent(const irr::SEvent& event);
	
	virtual void cd(const std::string& path);
	
};

//! Interface for retrieving icons to depict the items
class IItemSelectIconSource{
	
	public:
	
	virtual ~IItemSelectIconSource(){}
	
	virtual irr::video::ITexture* getAscendingIcon() = 0;
	
	virtual irr::video::ITexture* getDescendingIcon() = 0;
	
	virtual irr::video::ITexture* getMkdirIcon() = 0;
	
	virtual irr::video::ITexture* getFolderIcon() = 0;
	
	virtual irr::video::ITexture* getItemIcon(const IItemOrganizer::Item& item) = 0;
	
};

#endif
