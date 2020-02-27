#ifndef LoadSaveSettingsDialog_H_INCLUDED
#define LoadSaveSettingsDialog_H_INCLUDED

#include "ILanguagePhrases.h"

#include <IGUIElement.h>

#include "ForwardDeclarations.h"

#include <string>

class IniFile;
class AggregateGUIElement;
class Drawer2D;
class CMBox;
class LoadSaveSettingsDialog;
class EditBoxDialog;
class NameEditCallback;

class ILoadSaveSettingsCallback{

	public:
	
	enum Action{
		CANCEL,
		SAVE,
		LOAD,
		ACTION_COUNT
	};
	
	virtual ~ILoadSaveSettingsCallback(){}
	
	//! section must not be NULL in case of save/load, but can be NULL in case of cancel
	//! this method must only be called once by a single LoadSaveSettingsDialog
	//! iniDirty: true if the ini has already been changed by moving/deleting etc within the dialog
	virtual void OnResult(LoadSaveSettingsDialog* dialog, IniFile* ini, Action action, const char* section, bool iniDirty) = 0;

};

//! deletes itself after it has been closed by the user
class LoadSaveSettingsDialog : public irr::gui::IGUIElement{
	friend class NameEditCallback;
	
	public:
	
	enum Capabilities{
		OPEN,
		SAVE,
		OPEN_AND_SAVE,
		CAPS_COUNT
	};

	private:
	
	struct ListEntry{
		std::string path;//full section or folder prefix
		bool isFolder;
	};
	
	irr::IrrlichtDevice* device;
	IniFile* ini;
	ILoadSaveSettingsCallback* cbk;
	std::string exportPath;
	GUI* gui;
	
	CMBox* bImport;
	CMBox* bExport;
	CMBox* bDeleteAll;
	
	EditBoxDialog* bRename;
	int32_t renameIndex;
	
	CMBox* bDeleteOne;
	int32_t deleteIndex;
	
	CMBox* bOverwrite;
	
	EditBoxDialog* bNewDir;
	
	irr::s32 defaultAggId, invisibleAggId, regularAggId, noBorderAggId;
	irr::video::ITexture* folderIcon;
	irr::video::ITexture* settingsIcon;
	
	Drawer2D* drawer;
	
	irr::s32 deleteId, renameId;
	
	std::string currentPrefix;//without trailing /
	irr::core::array<ListEntry> entries;
	
	void fillList(const wchar_t* name = NULL);
	
	int32_t lastSelected;
	
	void addRowToList(irr::video::ITexture* fittingIcon, const std::string& indication, bool createButtons = true);
	
	NameEditCallback* nameCbk;
	
	Capabilities caps;
	
	bool iniDirty;
	
	void openOrSave(bool open);
	
	public:

	irr::gui::IGUIWindow* win;
	irr::gui::IGUIStaticText* ettitle;
	irr::gui::IGUIButton* ebcancel;
	irr::gui::IGUIButton* ebsaveload;
	irr::gui::IGUIButton* ebcreatefolder;
	irr::gui::IGUIButton* ebimport;
	irr::gui::IGUIButton* ebexport;
	irr::gui::IGUIButton* ebdeleteall;
	irr::gui::IGUIEditBox* eeName;
	irr::gui::IGUIButton* ebload;
	
	AggregateGUIElement* settingsList;
	irr::core::rect<irr::s32> slist;
	
	const ILanguagePhrases* phrases;
	
	//! the .*AggId parameters define the ids for the skin for the aggregation
	LoadSaveSettingsDialog(ILoadSaveSettingsCallback* cbk, irr::IrrlichtDevice* device, Drawer2D* drawer, IniFile* ini, const wchar_t* title, Capabilities caps, irr::s32 defaultAggId, irr::s32 noBorderAggId, irr::s32 invisibleAggId, irr::s32 regularAggId, const char* exportPath = NULL, irr::video::ITexture* folderIcon = NULL, irr::video::ITexture* settingsIcon = NULL, bool isModal = true, irr::s32 deleteId = -1, irr::s32 renameId = -1, const wchar_t* defaultFileName = L"");
	
	~LoadSaveSettingsDialog();
	
	bool OnEvent(const irr::SEvent& event);
	
	void OnPostRender(irr::u32 timeMs);
	
	void setLanguage(const ILanguagePhrases* phrases);
	
};

#endif
