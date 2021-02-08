#include <LoadSaveSettingsDialog.h>
#include <AggregateGUIElement.h>
#include <BeautifulGUIImage.h>
#include <AggregatableGUIElementAdapter.h>
#include <GUI.h>
#include <IniFile.h>
#include <StringHelpers.h>
#include <CMBox.h>
#include <EditBoxDialog.h>
#include <ConstantLanguagePhrases.h>
#include <timing.h>
#include <BeautifulGUIText.h>
#include <utf8.h>
#include <ItemSelectElement.h>

#include <IVideoDriver.h>
#include <IGUIWindow.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IrrlichtDevice.h>
#include <IGUIEditBox.h>

#include <iostream>
#include <sstream>
#include <set>
#include <cassert>

using namespace irr;
using namespace core;
using namespace gui;
using namespace video;

#define DOUBLE_CLICK_TIME 0.5//s

static const ConstantLanguagePhrases defaultPhrases({
	{L"LoadSaveSettingsDialog::IMPORT",L"Importing from: %s\nImporting may overwrite existing data. Do you wish to continue?"},
	{L"LoadSaveSettingsDialog::EXPORT",L"Exporting to: %s\nDo you wish to continue?"},
	{L"LoadSaveSettingsDialog::DELETE",L"Do you really want to delete the following item?:\n%s"},
	{L"LoadSaveSettingsDialog::DELETE_ALL",L"Do you really want to delete all items?\nPerhaps it is a good idea to press \"No\" and to create a backup first!"},
	{L"LoadSaveSettingsDialog::OVERWRITE",L"%s already exists.\nDo you want to overwrite this item?"},
	{L"LoadSaveSettingsDialog::INVALID_SELECTION",L"Error: Invalid filename."},
	{L"LoadSaveSettingsDialog::NOT_AVAILABLE",L"Error: Selected name not available."},
	{L"LoadSaveSettingsDialog::YES",L"Yes"},
	{L"LoadSaveSettingsDialog::NO",L"No"},
	{L"LoadSaveSettingsDialog::NEW_DIRECTORY",L"Please enter the name of the new directory:\nHint: Empty directories will not persist."},
	{L"LoadSaveSettingsDialog::OK",L"Ok"},
	{L"LoadSaveSettingsDialog::CANCEL",L"Cancel"},
	{L"LoadSaveSettingsDialog::RENAME",L"Please choose a new name/location:"},
	{L"LoadSaveSettingsDialog::BAD_CHAR_SEQUENCE",L"Error: Bad character sequence in name."},
	{L"LoadSaveSettingsDialog::EMPTY_NAMES",L"Error: Empty names are not possible."},
	{L"LoadSaveSettingsDialog::OPEN_BUTTON",L"Open"},
	{L"LoadSaveSettingsDialog::SAVE_BUTTON",L"Save"},
	{L"LoadSaveSettingsDialog::IMPORT_BUTTON",L"Import"},
	{L"LoadSaveSettingsDialog::EXPORT_BUTTON",L"Export"},
	{L"LoadSaveSettingsDialog::DELETE_ALL_BUTTON",L"Delete All"},
	{L"LoadSaveSettingsDialog::NEW_DIRECTORY_BUTTON",L"Create Folder"}
});

class NameEditCallback : public IEditBoxDialogCallback{

	private:
	
	LoadSaveSettingsDialog* lssDialog;

	public:
	
	NameEditCallback(LoadSaveSettingsDialog* lssDialog):lssDialog(lssDialog){}
	
	void OnResult(EditBoxDialog* dialog, const wchar_t* text, bool positivePressed){
		if(positivePressed){
			std::string newName = convertWStringToUtf8String(text);
			if(newName.empty()){
				new CMBox(lssDialog->device, lssDialog->phrases->getPhrase(L"LoadSaveSettingsDialog::EMPTY_NAMES", &defaultPhrases).c_str());
				return;
			}
			if(dialog==lssDialog->bRename){
				//Sanity checks:
				if(newName[0]!='/'){newName = std::string("/").append(newName);}
				while(newName[newName.size()-1]=='/'){newName = newName.substr(0,newName.size()-1);}
				if(newName.empty()){
					new CMBox(lssDialog->device, lssDialog->phrases->getPhrase(L"LoadSaveSettingsDialog::EMPTY_NAMES", &defaultPhrases).c_str());
					return;
				}else if(newName.find("..")!=std::string::npos || newName.find("//")!=std::string::npos || newName.find('\0')!=std::string::npos || newName.find('\n')!=std::string::npos){
					new CMBox(lssDialog->device, lssDialog->phrases->getPhrase(L"LoadSaveSettingsDialog::BAD_CHAR_SEQUENCE", &defaultPhrases).c_str());
					return;
				}
				//Renaming:
				LoadSaveSettingsDialog::ListEntry& e = lssDialog->entries[lssDialog->renameIndex];
				if(e.path.compare(newName)!=0){//don't rename if name not changed, otherwise problem with moveSection
					if(e.isFolder){
						std::string prefix = std::string(e.path).append("/");
						std::list<std::string> toRename;
						for(auto it = lssDialog->ini->data.begin(); it != lssDialog->ini->data.end(); ++it){
							if(isPrefixEqual(it->first, prefix)){
								toRename.push_back(it->first);
							}
						}
						for(auto it = toRename.begin(); it != toRename.end(); ++it){
							lssDialog->iniDirty = true;
							lssDialog->ini->moveSection(*it, std::string(newName).append(it->substr(e.path.size())));
						}
					}else{
						lssDialog->iniDirty = true;
						lssDialog->ini->moveSection(e.path, newName);
					}
					lssDialog->fillList();
				}
			}else if(dialog==lssDialog->bNewDir){
				if(newName.find("..")!=std::string::npos || newName.find(L'/')!=std::string::npos || newName.find('\0')!=std::string::npos || newName.find('\n')!=std::string::npos){
					new CMBox(lssDialog->device, lssDialog->phrases->getPhrase(L"LoadSaveSettingsDialog::BAD_CHAR_SEQUENCE", &defaultPhrases).c_str());
					return;
				}
				lssDialog->currentPrefix.append("/").append(newName);
				lssDialog->fillList();
			}
		}
		lssDialog->bRename = lssDialog->bNewDir = NULL;
	}
	
};

static const char* guiCodeString = 
"EleCount = 10\n"
"Ratio = 1\n"
"\n"
"[E0]\n"
"HAlign = Center\n"
"Id = ttitle\n"
"Rect = 0.015625,0.015625,0.984375,0.078125\n"
"Text = Title\n"
"Type = Text\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n"
"\n"
"[E1]\n"
"HAlign = \n"
"Id = bcancel\n"
"Rect = 0.015625,0.921875,0.328125,0.984375\n"
"Text = Cancel\n"
"Type = Button\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E2]\n"
"HAlign = \n"
"Id = bsaveload\n"
"Rect = 0.671875,0.921875,0.984375,0.984375\n"
"Text = Save\n"
"Type = Button\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E3]\n"
"HAlign = \n"
"Id = bcreatefolder\n"
"Rect = 0.765625,0.1875,0.984375,0.25\n"
"Text = Create\\ Folder\n"
"Type = Button\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E4]\n"
"HAlign = \n"
"Id = bimport\n"
"Rect = 0.015625,0.1875,0.234375,0.25\n"
"Text = Import\n"
"Type = Button\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E5]\n"
"HAlign = \n"
"Id = bexport\n"
"Rect = 0.265625,0.1875,0.484375,0.25\n"
"Text = Export\n"
"Type = Button\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E6]\n"
"HAlign = \n"
"Id = bdeleteall\n"
"Rect = 0.515625,0.1875,0.734375,0.25\n"
"Text = Delete\\ All\n"
"Type = Button\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E7]\n"
"Default = \n"
"EditType = STRING_EDIT\n"
"HAlign = Left\n"
"Id = eName\n"
"Rect = 0.015625,0.09375,0.984375,0.15625\n"
"Text = \n"
"Type = EditBox\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n"
"\n"
"[E8]\n"
"HAlign = \n"
"Id = slist\n"
"Rect = 0.015625,0.265625,0.984375,0.90625\n"
"Text = \n"
"Type = SpecialRectangle\n"
"VAlign = \n"
"VisibilityDomain = 0\n"
"\n"
"[E9]\n"
"HAlign = Center\n"
"Id = bload\n"
"Rect = 0.34375,0.921875,0.65625,0.984375\n"
"Text = Open\n"
"Type = Button\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n";

static const IItemOrganizer::Item dummyItem{false, "settings.ini", {}, NULL, 0};

LoadSaveSettingsDialog::LoadSaveSettingsDialog(ILoadSaveSettingsCallback* cbk, irr::IrrlichtDevice* device, Drawer2D* drawer, IniFile* ini, const wchar_t* title, Capabilities caps, irr::s32 defaultAggId, irr::s32 noBorderAggId, irr::s32 invisibleAggId, irr::s32 regularAggId, IItemSelectIconSource* iconsource, const char* exportPath, bool isModal, irr::s32 deleteId, irr::s32 renameId, const wchar_t* defaultFileName, const IndicationFunction* calculateIndication):
	IGUIElement(EGUIET_ELEMENT, device->getGUIEnvironment(), device->getGUIEnvironment()->getRootGUIElement(), -1, rect<s32>(0,0,device->getVideoDriver()->getScreenSize().Width,device->getVideoDriver()->getScreenSize().Height)),
	device(device),
	ini(ini),
	cbk(cbk),
	exportPath(exportPath!=NULL?(isSuffixEqual(exportPath,".ini")?exportPath:(std::string(exportPath).append(".ini"))):"data.ini"),
	defaultAggId(defaultAggId),
	invisibleAggId(invisibleAggId),
	regularAggId(regularAggId),
	noBorderAggId(noBorderAggId),
	folderIcon(iconsource->getFolderIcon()),
	settingsIcon(iconsource->getItemIcon(dummyItem)),
	drawer(drawer),
	deleteId(deleteId),
	renameId(renameId),
	caps(caps),
	iniDirty(false),
	calculateIndication(calculateIndication?*calculateIndication:[](const std::wstring& indication, IniFile* ini ,const std::string& section){return indication;}),
	iconsource(iconsource){
	IniFile guiIni;
	guiIni.setFromString(guiCodeString);
	irr::core::dimension2d<irr::u32> dim = Environment->getVideoDriver()->getScreenSize();
	win = addWindowForRatio(Environment, &guiIni, irr::core::rect<irr::s32>(0,0,dim.Width,dim.Height), isModal, this);
	win->getCloseButton()->setVisible(false);
	win->setDrawTitlebar(false);
	win->setDraggable(false);
	gui = new GUI(Environment, &guiIni, irr::core::rect<irr::s32>(0,0,win->getRelativePosition().getWidth(),win->getRelativePosition().getHeight()), win, true);
	ettitle = (IGUIStaticText*)gui->getElement("ttitle");
	ettitle->setText(title);
	ebcancel = (IGUIButton*)gui->getElement("bcancel");
	ebsaveload = (IGUIButton*)gui->getElement("bsaveload");
	ebcreatefolder = (IGUIButton*)gui->getElement("bcreatefolder");
	ebimport = (IGUIButton*)gui->getElement("bimport");
	ebimport->setVisible(exportPath!=NULL);
	ebexport = (IGUIButton*)gui->getElement("bexport");
	ebexport->setVisible(exportPath!=NULL);
	ebdeleteall = (IGUIButton*)gui->getElement("bdeleteall");
	eeName = (IGUIEditBox*)gui->getElement("eName");
	ebload = (IGUIButton*)gui->getElement("bload");
	ebload->setVisible(caps==OPEN_AND_SAVE);
	slist = gui->getSpecialRectangle("slist");
	settingsList = NULL;
	currentPrefix = "";
	fillList(defaultFileName);
	bDeleteOne = bOverwrite = bDeleteAll = NULL;
	bNewDir = bRename = NULL;
	setLanguage(&defaultPhrases);
	nameCbk = new NameEditCallback(this);
	bOverwrite = bDeleteOne = NULL;
	lastSelectionChange = 0;
	drop();//drop since the valid reference is hold by the parent (root element)
}

LoadSaveSettingsDialog::~LoadSaveSettingsDialog(){
	delete gui;
	delete nameCbk;
	//children get automatically removed
}

void LoadSaveSettingsDialog::setIndicationOverideFunction(const IndicationFunction& calculateIndication){
	this->calculateIndication = calculateIndication;
}


void LoadSaveSettingsDialog::setLanguage(const ILanguagePhrases* phrases){
	this->phrases = phrases;
	ebsaveload->setText((caps==OPEN?phrases->getPhrase(L"LoadSaveSettingsDialog::OPEN_BUTTON", &defaultPhrases):phrases->getPhrase(L"LoadSaveSettingsDialog::SAVE_BUTTON", &defaultPhrases)).c_str());
	ebload->setText(phrases->getPhrase(L"LoadSaveSettingsDialog::OPEN_BUTTON", &defaultPhrases).c_str());
	ebcancel->setText(phrases->getPhrase(L"LoadSaveSettingsDialog::CANCEL", &defaultPhrases).c_str());
	ebimport->setText(phrases->getPhrase(L"LoadSaveSettingsDialog::IMPORT_BUTTON", &defaultPhrases).c_str());
	ebexport->setText(phrases->getPhrase(L"LoadSaveSettingsDialog::EXPORT_BUTTON", &defaultPhrases).c_str());
	ebdeleteall->setText(phrases->getPhrase(L"LoadSaveSettingsDialog::DELETE_ALL_BUTTON", &defaultPhrases).c_str());
	ebcreatefolder->setText(phrases->getPhrase(L"LoadSaveSettingsDialog::NEW_DIRECTORY_BUTTON", &defaultPhrases).c_str());
}

void LoadSaveSettingsDialog::addRowToList(irr::video::ITexture* fittingIcon, const std::string& indication, bool createButtons){
	AggregateGUIElement* row = new AggregateGUIElement(Environment, .15, 1.f, .15, 1.f, false, true, false, {
		new EmptyGUIElement(Environment, .025f, 1.f/1.f, false, false, defaultAggId),
		fittingIcon!=NULL?
			(IAggregatableGUIElement*)new AggregateGUIElement(Environment, .2, 1.f, .2, 1.f, false, false, false, {
				new EmptyGUIElement(Environment, .1f, 1.f/1.f, false, false, defaultAggId),
				new BeautifulGUIImage(drawer, fittingIcon, Environment, .9f, true, defaultAggId, SColor(255,255,255,255)),
				new EmptyGUIElement(Environment, .1f, 1.f/1.f, false, false, defaultAggId)
			}, {}, false, invisibleAggId)
		:
			(IAggregatableGUIElement*)new EmptyGUIElement(Environment, .2f, 1.f/1.f, false, false, defaultAggId),
		new EmptyGUIElement(Environment, .025f, 1.f/1.f, false, false, defaultAggId),
		new BeautifulGUIText(calculateIndication(convertUtf8ToWString(indication),ini,std::string(currentPrefix).append(indication[0]=='/'?"":"/").append(indication)).c_str(), Environment->getSkin()->getColor(EGDC_BUTTON_TEXT), 0.f, NULL, false, true, Environment, 1.3f),
		createButtons?
			(IAggregatableGUIElement*)new AggregatableGUIElementAdapter(Environment, .2f, 7.f/4.f, true, Environment->addButton(rect<s32>(0,0,0,0), NULL, renameId, renameId<0?L"Move":L""), false, defaultAggId)//Attention: indices in OnEvent must match the order, else => crash
		:
			(IAggregatableGUIElement*)new EmptyGUIElement(Environment, .2f, 7.f/4.f, true, false, defaultAggId),
		createButtons?
			(IAggregatableGUIElement*)new AggregatableGUIElementAdapter(Environment, .2f, 7.f/4.f, true, Environment->addButton(rect<s32>(0,0,0,0), NULL, deleteId, deleteId<0?L"Delete":L""), false, defaultAggId)
		:
			(IAggregatableGUIElement*)new EmptyGUIElement(Environment, .2f, 7.f/4.f, true, false, defaultAggId),
	}, {}, true, noBorderAggId);
	settingsList->addSubElement(row);
}

static int32_t findLastSlash(const std::string& s){
	int32_t lastSlash = s.size()-1;
	for(; lastSlash>=0; lastSlash--){if(s[lastSlash]=='/'){break;}}
	return lastSlash;
}

void LoadSaveSettingsDialog::fillList(const wchar_t* name){
	lastSelected = -1;
	if(settingsList){settingsList->remove();}
	settingsList = new AggregateGUIElement(Environment, 1.f, 1.f, 1.f, 1.f, false, false, true, {}, {}, false, regularAggId, NULL, win, slist);
	IniIterator* it = ini->createNewIterator();
	entries.clear();
	entries.reallocate(ini->data.size()+1);//max size +1 because of potential ..
	if(name){eeName->setText(name);}else{eeName->setText(L"");}
	std::set<std::string> addedFolders;//set to prevent duplicate adding of folders
	if(!currentPrefix.empty()){
		int32_t lastSlash = findLastSlash(currentPrefix);
		assert(lastSlash>=0);
		entries.push_back(ListEntry{currentPrefix.substr(0, lastSlash), true});
		addRowToList(folderIcon, "..", false);
	}
	std::string prefixWithSlash = std::string(currentPrefix).append("/");
	while(it->isSectionAvail()){
		irr::video::ITexture* fittingIcon = settingsIcon;
		const std::string& section = it->getCurrentSection();
		if(isPrefixEqual(section, prefixWithSlash) && section.size()>currentPrefix.size()){
			size_t nextSlash = section.find('/', currentPrefix.size()+1);
			std::string indication;
			bool mustAdd = true;
			if(nextSlash!=std::string::npos){
				fittingIcon = folderIcon;
				indication = section.substr(currentPrefix.size()+1,nextSlash-currentPrefix.size()-1);
				std::string folderPath = section.substr(0, nextSlash);
				mustAdd = addedFolders.find(folderPath)==addedFolders.end();
				if(mustAdd){
					addedFolders.insert(folderPath);
					entries.push_back(ListEntry{folderPath, true});
				}
			}else{
				indication = section.substr(currentPrefix.size()+1, std::string::npos);
				entries.push_back(ListEntry{section, false});
			}
			if(mustAdd){
				addRowToList(fittingIcon, indication);
			}
		}
		it->gotoNextSection();
	}
	delete it;
}

void LoadSaveSettingsDialog::openOrSave(bool open){
	std::string text = convertWStringToUtf8String(eeName->getText());
	if(text.empty() || text.find("..")!=std::string::npos || text[text.size()-1]=='/'){
		new CMBox(device, phrases->getPhrase(L"LoadSaveSettingsDialog::INVALID_SELECTION", &defaultPhrases).c_str(), .75f, .75f, phrases->getPhrase(L"LoadSaveSettingsDialog::OK", &defaultPhrases).c_str());
	}else if(text.find("//")!=std::string::npos || text.find('\0')!=std::string::npos || text.find('\n')!=std::string::npos){
		new CMBox(device, phrases->getPhrase(L"LoadSaveSettingsDialog::BAD_CHAR_SEQUENCE", &defaultPhrases).c_str(), .75f, .75f, phrases->getPhrase(L"LoadSaveSettingsDialog::OK", &defaultPhrases).c_str());
	}else{
		text = std::string(currentPrefix).append(text[0]=='/'?"":"/").append(text);
		if(ini->isSectionAvailable(text)){
			if(!open){
				bOverwrite = new CMBox(device, phrases->getPhrase(L"LoadSaveSettingsDialog::OVERWRITE", {convertUtf8ToWString(text)}, &defaultPhrases).c_str(), 0.9f, 0.9f, phrases->getPhrase(L"LoadSaveSettingsDialog::YES", &defaultPhrases).c_str(), phrases->getPhrase(L"LoadSaveSettingsDialog::NO", &defaultPhrases).c_str());
				addChild(bOverwrite);
				return;
			}
		}else if(open){
			new CMBox(device, phrases->getPhrase(L"LoadSaveSettingsDialog::NOT_AVAILABLE", &defaultPhrases).c_str(), .75f, .75f, phrases->getPhrase(L"LoadSaveSettingsDialog::OK", &defaultPhrases).c_str());
			return;
		}
		cbk->OnResult(this, ini, open?(ILoadSaveSettingsCallback::LOAD):(ILoadSaveSettingsCallback::SAVE), text.c_str(), iniDirty);
		remove();
	}
}

bool LoadSaveSettingsDialog::OnEvent(const SEvent& event){
	if(event.EventType == EET_GUI_EVENT){
		const SEvent::SGUIEvent& g = event.GUIEvent;
		if(g.EventType==EGET_BUTTON_CLICKED){
			if(g.Caller==ebcancel){
				cbk->OnResult(this, ini, ILoadSaveSettingsCallback::CANCEL, NULL, iniDirty);
				remove();
				return true;
			}else if(g.Caller==ebsaveload){
				openOrSave(caps==OPEN);
				return true;
			}else if(g.Caller==ebload){
				openOrSave(true);
				return true;
			}else if(g.Caller==ebimport){
				createItemSelectElement(device, drawer, exportPath, regularAggId, noBorderAggId, invisibleAggId, -1, false, [this](IItemSelectCallback::Action action, IItemOrganizer::Item* item, const std::string& absolutePath, ItemSelectElement* ele, IItemOrganizer* organizer){
					if(action==IItemSelectCallback::OPEN){
						IniFile importedIni(absolutePath);
						IniIterator* it = importedIni.createNewIterator();
						while(it->isSectionAvail()){//merge imported ini and overwrite without asking
							while(it->isValueAvail()){
								iniDirty = true;
								ini->set(it->getCurrentSection(), it->getCurrentKey(), it->getCurrentValue());
								it->gotoNextValue();
							}
							it->gotoNextSection();
						}
						delete it;
						fillList();
					}
				}, iconsource, .75f, .75f, phrases, true, std::regex(".*.ini"));//TODO richtige icons
				return true;
			}else if(g.Caller==ebexport){
				createItemSelectElement(device, drawer, exportPath, regularAggId, noBorderAggId, invisibleAggId, -1, true, [this](IItemSelectCallback::Action action, IItemOrganizer::Item* item, const std::string& absolutePath, ItemSelectElement* ele, IItemOrganizer* organizer){
					if(action==IItemSelectCallback::SAVE){
						ini->save(absolutePath);
					}
				}, iconsource, .75f, .75f, phrases, true, std::regex(".*.ini"));
				return true;
			}else if(g.Caller==ebdeleteall){
				bDeleteAll = new CMBox(device, phrases->getPhrase(L"LoadSaveSettingsDialog::DELETE_ALL", &defaultPhrases).c_str(), 0.9f, 0.9f, phrases->getPhrase(L"LoadSaveSettingsDialog::YES", &defaultPhrases).c_str(), phrases->getPhrase(L"LoadSaveSettingsDialog::NO", &defaultPhrases).c_str());
				addChild(bDeleteAll);
				return true;
			}else if(g.Caller==ebcreatefolder){
				bNewDir = new EditBoxDialog(nameCbk, device, phrases->getPhrase(L"LoadSaveSettingsDialog::NEW_DIRECTORY", &defaultPhrases).c_str(), phrases->getPhrase(L"LoadSaveSettingsDialog::OK", &defaultPhrases).c_str(),  phrases->getPhrase(L"LoadSaveSettingsDialog::CANCEL", &defaultPhrases).c_str(), L"", true);
				addChild(bNewDir);
				return true;
			}else{
				for(u32 i=0; i<settingsList->getSubElementCount(); i++){
					AggregateGUIElement* row = (AggregateGUIElement*)(settingsList->getSubElement(i));
					IGUIElement* bmove = getFirstGUIElementChild(row->getSubElement(4));//Attention: indices in OnEvent must match the order, else => crash
					IGUIElement* bdelete = getFirstGUIElementChild(row->getSubElement(5));
					if(g.Caller==bmove){
						bRename = new EditBoxDialog(nameCbk, device, phrases->getPhrase(L"LoadSaveSettingsDialog::RENAME", &defaultPhrases).c_str(), phrases->getPhrase(L"LoadSaveSettingsDialog::OK", &defaultPhrases).c_str(),  phrases->getPhrase(L"LoadSaveSettingsDialog::CANCEL", &defaultPhrases).c_str(), convertUtf8ToWString(entries[i].path).c_str(), true);
						addChild(bRename);
						renameIndex = i;
						return true;
					}else if(g.Caller==bdelete){
						bDeleteOne = new CMBox(device, phrases->getPhrase(L"LoadSaveSettingsDialog::DELETE", {convertUtf8ToWString(entries[i].path)}, &defaultPhrases).c_str(), 0.75, 0.75, phrases->getPhrase(L"LoadSaveSettingsDialog::YES", &defaultPhrases).c_str(), phrases->getPhrase(L"LoadSaveSettingsDialog::NO", &defaultPhrases).c_str());
						deleteIndex = i;
						addChild(bDeleteOne);
						return true;
					}
				}
			}
		}else if(g.EventType==EGET_MESSAGEBOX_YES){
			if(g.Caller==(IGUIElement*)bDeleteAll){
				iniDirty = true;
				ini->data.clear();
				fillList();
				bDeleteAll = NULL;
				return true;
			}else if(g.Caller==(IGUIElement*)bDeleteOne){
				ListEntry& e = entries[deleteIndex];
				if(e.isFolder){
					auto it = ini->data.begin();
					while(it != ini->data.end()){
						if(isPrefixEqual(it->first, e.path)){
							iniDirty = true;
							it = ini->data.erase(it);
						}else{
							++it;
						}
					}
				}else{
					iniDirty = true;
					ini->removeSection(e.path);
				}
				fillList();
				bDeleteOne = NULL;
				return true;
			}else if(g.Caller==(IGUIElement*)bOverwrite){
				std::string text = convertWStringToUtf8String(eeName->getText());
				text = std::string(currentPrefix).append(text[0]=='/'?"":"/").append(text);
				cbk->OnResult(this, ini, ILoadSaveSettingsCallback::SAVE, text.c_str(), iniDirty);
				remove();
				bOverwrite = NULL;
				return true;
			}
		}else if(g.EventType==EGET_MESSAGEBOX_NO){
			bDeleteOne = bOverwrite = bDeleteAll = NULL;
		}else if(g.EventType==EGET_LISTBOX_CHANGED){
			s32 nowSelected = -1;
			for(u32 i=0; i<settingsList->getSubElementCount(); i++){
				AggregateGUIElement* row = (AggregateGUIElement*)(settingsList->getSubElement(i));
				if(row->isActive()){
					nowSelected = i;
					break;
				}
			}
			if(nowSelected!=lastSelected){
				if(nowSelected>=0){
					ListEntry& e = entries[nowSelected];
					if(e.isFolder){
						currentPrefix = e.path;
						fillList(L"");
						lastSelected = nowSelected = -1;
						return true;
					}else{
						int32_t lastSlash = findLastSlash(e.path);
						eeName->setText(convertUtf8ToWString(lastSlash>=0?e.path.substr(lastSlash+1,std::string::npos):e.path).c_str());
					}
				}
				double t = getSecs();
				if((nowSelected==-1 || lastSelected==-1) && t-lastSelectionChange<DOUBLE_CLICK_TIME){
					openOrSave(true);
				}
				lastSelectionChange = t;
			}
			lastSelected = nowSelected;
			return true;
		}
	}else if(event.EventType==EET_KEY_INPUT_EVENT){
		const SEvent::SKeyInput& k = event.KeyInput;
		if(!k.PressedDown){//Release
			if(k.Key==KEY_BROWSER_BACK){
				cbk->OnResult(this, ini, ILoadSaveSettingsCallback::CANCEL, NULL, iniDirty);
				remove();
			}
		}
		return true;
	}
	return irr::gui::IGUIElement::OnEvent(event);
}

void LoadSaveSettingsDialog::OnPostRender(irr::u32 timeMs){
	IGUIElement::OnPostRender(timeMs);
}
