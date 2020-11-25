#include "ItemSelectElement.h"
#include "CMBox.h"
#include "EditConstants.h"
#include "ConstantLanguagePhrases.h"
#include "BeautifulGUIText.h"
#include "mathUtils.h"
#include "AggregateGUIElement.h"
#include "BeautifulGUIButton.h"
#include "BeautifulGUIImage.h"
#include "EditBoxDialog.h"

#include <utf8.h>

#include <IrrlichtDevice.h>
#include <IGUIWindow.h>
#include <IGUIButton.h>
#include <IGUIEditBox.h>
#include <IGUIFont.h>
#include <rect.h>
#include <SColor.h>
#include <IGUISkin.h>

#include <sstream>
#include <iostream>

using namespace irr;
using namespace core;
using namespace gui;
using namespace video;

#define BUTTON_WIDHT_PART (0.0625*0.79057)

//static void printItemVector(const std::vector<IItemOrganizer::Item*>& v){
//	for(uint32_t i=0; i<v.size(); i++){
//		IItemOrganizer::Item* item = v[i];
//		for(uint32_t j=0; j<item->fields.size(); j++){
//			std::cout << "field[" << j << "] = \"" << convertWStringToUtf8String(item->fields[j]) << "\"" << std::endl;
//		}
//	}
//}

static const ConstantLanguagePhrases defaultPhrases({
	{L"ItemSelectElement::NAME",L"Name:"},
	{L"ItemSelectElement::PLACES",L"Places:"},
	{L"ItemSelectElement::CANCEL",L"Cancel"},
	{L"ItemSelectElement::OPEN",L"Open"},
	{L"ItemSelectElement::SAVE",L"Save"},
	{L"ItemSelectElement::MKDIR_ERROR",L"Error while creating new directory."},
	{L"ItemSelectElement::MKDIR_DESC",L"Please enter the name of the new directory:"},
	{L"ItemSelectElement::OK",L"OK"},
	{L"ItemSelectElement::EMPTY_FILENAME",L"Error: No filename specified."},
	{L"ItemSelectElement::ASK_OVERWRITE",L"Do you want to overwrite the existing file?"},
	{L"ItemSelectElement::YES",L"Yes"},
	{L"ItemSelectElement::NO",L"No"},
	{L"ItemSelectElement::NO_SELECTION",L"Error: No file selected."}
});

class ItemSelectEditBoxCallback : public IEditBoxDialogCallback{

	public:
	
	ItemSelectElement* ele;
	
	ItemSelectEditBoxCallback(ItemSelectElement* ele):ele(ele){}
	
	void OnResult(EditBoxDialog* dialog, const wchar_t* text, bool positivePressed){
		if(positivePressed){
			std::string dirname = convertWStringToUtf8String(text);
			if(!dirname.empty()){
				if(ele->organizer->mkdir(dirname)){
					ele->cd(dirname);
				}else{
					new CMBox(ele->device, ele->lang->getPhrase(L"ItemSelectElement::MKDIR_ERROR", &defaultPhrases));
				}
			}
		}
	}

};

ItemSelectElement::ItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, const std::wstring& defaultSaveFileName, IItemOrganizer* organizer, IItemSelectCallback* itemcbk, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 invisibleAggregationID, irr::s32 mkdirButtonID, bool isSaveDialog, IItemSelectIconSource* source, const std::initializer_list<IAggregatableGUIElement*>& subElements, irr::f32 w, irr::f32 h, const ILanguagePhrases* phrases, bool modal):
	IGUIElement(irr::gui::EGUIET_ELEMENT, device->getGUIEnvironment(), NULL, -1, modal?rect<s32>(0,0,device->getVideoDriver()->getScreenSize().Width,device->getVideoDriver()->getScreenSize().Height):rect<s32>(0,0,0,0)){
	this->device = device;
	this->organizer = organizer;
	this->itemcbk = itemcbk;
	this->isSaveDialog = isSaveDialog;
	this->aggregationID = aggregationID;
	this->source = source;
	this->listElementAggregationID = listElementAggregationID;
	this->invisibleAggregationID = invisibleAggregationID;
	this->drawer = drawer;
	env = device->getGUIEnvironment();
	env->getRootGUIElement()->addChild(this);
	driver = device->getVideoDriver();
	lang = phrases==NULL?&defaultPhrases:phrases;
	u32 ww = driver->getScreenSize().Width;
	u32 wh = driver->getScreenSize().Height;
	u32 pw = w*ww;
	u32 ph = h*wh;
	double sqrtArea = sqrt(ww*wh);
	buttonHeight = (s32)(BUTTON_WIDHT_PART*sqrtArea);
	cbk = new ItemSelectEditBoxCallback(this);
	IGUIWindow* win = env->addWindow(rect<s32>(ww/2-pw/2, wh/2-ph/2, ww/2+pw/2, wh/2+ph/2), modal, L"", this, -1);
	win->getCloseButton()->setVisible(false); win->setDrawTitlebar(false); win->setNotClipped(true);
	win->setDraggable(false);
	bringToFrontRecursive(win);
	s32 padding = 0.015*sqrtArea;
	IGUIFont* font = env->getSkin()->getFont();
	const std::wstring& nameLabel = lang->getPhrase(L"ItemSelectElement::NAME", &defaultPhrases);
	dimension2d<u32> nameLabelSize = font->getDimension(nameLabel.c_str());
	buttonHeight = Max(buttonHeight, (s32)nameLabelSize.Height);
	SColor textColor = env->getSkin()->getColor(EGDC_BUTTON_TEXT);
	s32 startY = 0;
	if(isSaveDialog){
		new BeautifulGUIText(nameLabel.c_str(), textColor, 0.f, NULL, true, true, env, 1.f, -1, 1.f, NULL, win, rect<s32>(padding,padding,padding+nameLabelSize.Width, padding+buttonHeight));
		nameEdit = env->addEditBox(defaultSaveFileName.c_str(), rect<s32>(2*padding+nameLabelSize.Width, padding, pw-padding, padding+buttonHeight), true, win, STRING_EDIT);
		startY = padding+buttonHeight;
		env->setFocus(nameEdit);
		//put cursor to the end:
		SEvent event;
		event.EventType = EET_KEY_INPUT_EVENT;
		event.KeyInput = SEvent::SKeyInput{L'\0', KEY_END, 0, true, false, false};
		nameEdit->OnEvent(event);
		event.KeyInput.PressedDown = false;
		nameEdit->OnEvent(event);
	}else{
		nameEdit = NULL;
		env->setFocus(win);
	}
	parent = win;
	pathAgg = placesAgg = filesAgg = filesAndHeaderAgg = NULL;
	negative = positive = NULL;
	overwritebox = NULL;
	selectedIndex = -1;
	createPlacesGUI(rect<s32>(padding, padding/2+startY, pw/4-padding, ph-3*padding/2-buttonHeight), padding/2, win, buttonHeight);
	navBarRect = rect<s32>(pw/4, padding/2+startY, pw-padding, padding+buttonHeight+startY);
	createNavigationBar(isSaveDialog, mkdirButtonID);
	sortIndex = 0; sortAscending = true;
	fileListRect = rect<s32>(pw/4, 3*padding/2+buttonHeight+startY, pw-padding, ph-3*padding/2-buttonHeight);
	createFileList();
	new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, true, false, {
		new EmptyGUIElement(env, 0.05f, 1.f, false, false, -1),
		(subElements.size()==0?
			((IAggregatableGUIElement*)new EmptyGUIElement(env, 0.6f, 1.f, false, false, -1)):
			((IAggregatableGUIElement*)new AggregateGUIElement(env, 0.6f, 1.f, 0.6f, 1.f, false, true, false, subElements, {}, false, listElementAggregationID))),
		new EmptyGUIElement(env, 0.05f, 1.f, false, false, -1),
		negative = new BeautifulGUIButton(env, 0.13f, 1.f, false, true, false,{
			new BeautifulGUIText(lang->getPhrase(L"ItemSelectElement::CANCEL", &defaultPhrases).c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, -1),
		new EmptyGUIElement(env, 0.04f, 1.f, false, false, -1),
		positive = new BeautifulGUIButton(env, 0.13f, 1.f, false, true, false,{
			new BeautifulGUIText(isSaveDialog?(lang->getPhrase(L"ItemSelectElement::SAVE", &defaultPhrases).c_str()):(lang->getPhrase(L"ItemSelectElement::OPEN", &defaultPhrases).c_str()), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, -1)
	}, {}, false, invisibleAggregationID, NULL, win, rect<s32>(padding, ph-padding-buttonHeight, pw-padding, ph-padding));
	//printItemVector(organizer->ls(1));
}

ItemSelectElement::ItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, IItemOrganizer* organizer, IItemSelectCallback* itemcbk, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 invisibleAggregationID, IItemSelectIconSource* source, const irr::core::rect<irr::s32>& rectangle, irr::f32 placesPart, irr::f32 pathPart, const ILanguagePhrases* phrases, irr::gui::IGUIElement* parent):
	IGUIElement(irr::gui::EGUIET_ELEMENT, device->getGUIEnvironment(), NULL, -1, rectangle){
	this->device = device;
	this->organizer = organizer;
	this->itemcbk = itemcbk;
	this->isSaveDialog = false;
	this->aggregationID = aggregationID;
	this->source = source;
	this->listElementAggregationID = listElementAggregationID;
	this->invisibleAggregationID = invisibleAggregationID;
	this->drawer = drawer;
	env = device->getGUIEnvironment();
	if(parent==NULL){parent = env->getRootGUIElement();}
	parent->addChild(this);
	this->parent = this;
	driver = device->getVideoDriver();
	lang = phrases==NULL?&defaultPhrases:phrases;
	u32 ww = driver->getScreenSize().Width;
	u32 wh = driver->getScreenSize().Height;
	double sqrtArea = sqrt(ww*wh);
	buttonHeight = (s32)(BUTTON_WIDHT_PART*sqrtArea);
	s32 padding = 0.015*sqrtArea;
	nameEdit = NULL;
	pathAgg = placesAgg = filesAgg = filesAndHeaderAgg = NULL;
	negative = positive = NULL;
	overwritebox = NULL;
	selectedIndex = -1;
	s32 placesLowerY = placesPart*rectangle.getHeight();
	s32 navBarLowerY = placesLowerY+pathPart*rectangle.getHeight();
	createPlacesGUI(rect<s32>(0, 0, rectangle.getWidth(), placesLowerY), padding/2, this, buttonHeight);
	navBarRect = rect<s32>(0, placesLowerY, rectangle.getWidth(), navBarLowerY);
	createNavigationBar(false, -1);
	sortIndex = 0; sortAscending = true;
	fileListRect = rect<s32>(0, navBarLowerY, rectangle.getWidth(), rectangle.getHeight());
	createFileList();
}

ItemSelectElement::~ItemSelectElement(){
	delete cbk;
}

void ItemSelectElement::createPlacesGUI(const irr::core::rect<irr::s32>& r, irr::s32 padding, irr::gui::IGUIElement* parent, irr::s32 buttonHeight){
	IGUIFont* font = env->getSkin()->getFont();
	const std::wstring& placesLabel = lang->getPhrase(L"ItemSelectElement::PLACES", &defaultPhrases);
	dimension2d<u32> placesLabelSize = font->getDimension(placesLabel.c_str());
	SColor textColor = env->getSkin()->getColor(EGDC_BUTTON_TEXT);
	new BeautifulGUIText(placesLabel.c_str(), textColor, 0.f, NULL, true, true, env, 1.f, -1, 1.f, NULL, parent, rect<s32>(r.UpperLeftCorner.X, r.UpperLeftCorner.Y+padding, r.LowerRightCorner.X, r.UpperLeftCorner.Y+placesLabelSize.Height+2*padding));
	placesAgg = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, false, true, {}, {}, false, aggregationID, NULL, parent, rect<s32>(r.UpperLeftCorner.X, r.UpperLeftCorner.Y+placesLabelSize.Height+3*padding, r.LowerRightCorner.X, r.LowerRightCorner.Y));
	const std::vector<IItemOrganizer::Place>& p = organizer->getPlaces();
	f32 space = 1.5f*((f32)buttonHeight)/((f32)r.getHeight());
	for(uint32_t i=0; i<p.size(); i++){
		placesAgg->addSubElement(new AggregateGUIElement(env, space, 1.f, space, 1.f, false, true, false, {
			new BeautifulGUIText(p[i].indication.c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, true, listElementAggregationID));
	}
}

void ItemSelectElement::createNavigationBar(bool createMkdirButton, irr::s32 mkdirButtonID){
	pathAgg = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, true, true, {}, {}, false, aggregationID, NULL, parent, createMkdirButton?rect<s32>(navBarRect.UpperLeftCorner.X, navBarRect.UpperLeftCorner.Y, navBarRect.LowerRightCorner.X-3*navBarRect.getHeight()/2, navBarRect.LowerRightCorner.Y):navBarRect);
	if(createMkdirButton){
		mkdirBut = new BeautifulGUIButton(env, 1.f, 1.f, false, true, false, {
				new BeautifulGUIImage(drawer, source->getMkdirIcon(), env, 1.f, true, -1)
		}, {}, -1, NULL, parent, rect<s32>(navBarRect.LowerRightCorner.X-navBarRect.getHeight(), navBarRect.UpperLeftCorner.Y, navBarRect.LowerRightCorner.X, navBarRect.LowerRightCorner.Y));
	}else{
		mkdirBut = NULL;
	}
	const std::string& d = organizer->pwd();
	//separate path into tokens:
	std::list<std::string> tokens;
	std::stringstream ss;
	for(uint32_t i=0; i<d.size(); i++){
		char c = d[i];
		if(c=='/' || c=='\\'){
			tokens.push_back(ss.str());
			ss.str("");
		}else{
			ss << c;
		}
	}
	tokens.push_back(ss.str());
	//clean tokens
	auto it = tokens.begin();
	if(it->empty()){*it = "/";}
	++it;
	while(it!=tokens.end()){
		if(it->empty()){
			it = tokens.erase(it);
		}else{
			++it;
		}
	}
	//create buttons for tokens
	IGUIFont* font = env->getSkin()->getFont();
	f32 totalWidth = (f32)navBarRect.getWidth();
	SColor textColor = env->getSkin()->getColor(EGDC_BUTTON_TEXT);
	std::stringstream curpath;
	for(auto it=tokens.begin(); it!=tokens.end(); ++it){
		if(it!=tokens.begin()){
			#if _WIN32
			curpath << '\\';
			#else
			curpath << '/';
			#endif
		}
		curpath << *it;
		std::wstring text = convertUtf8ToWString(*it);
		dimension2d<u32> tsize = font->getDimension(text.c_str());
		f32 space = 0.05f+((f32)tsize.Width)/totalWidth;
//		if(it!=tokens.begin()){
//			pathAgg->addSubElement(new EmptyGUIElement(env, 0.01f, 1.f, false, false, -1));
//		}
		BeautifulGUIButton* but = new BeautifulGUIButton(env, space, 1.f, false, true, false,{
			new BeautifulGUIText(text.c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, -1);
		pathAgg->addSubElement(but);
		button2path[but] = curpath.str();
	}
	pathAgg->updateAbsolutePosition();
	pathAgg->setScrollPosition(1.f);
}

void ItemSelectElement::createFileList(){
	filesAndHeaderAgg = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, false, false, {}, {}, false, aggregationID, NULL, parent, fileListRect);
	const std::vector<std::wstring>& labels = organizer->getItemFieldLabels();
	items = organizer->ls(sortIndex, sortAscending);
	//if(!sortAscending){std::reverse(std::begin(items), std::end(items));}
	assert(labels.size()>=1);
	//Calculate space requirements:
	f32 vspaceHeader = 1.25f*((f32)buttonHeight)/((f32)fileListRect.getHeight());
	f32 vspace = 1.25f*((f32)buttonHeight)/((f32)fileListRect.getHeight()-buttonHeight);
	f32 lwidth = fileListRect.getWidth();
	f32 iconspace = ((f32)buttonHeight)/((f32)lwidth);
	f32 maxhspace = Max(0.1f,(1.f-iconspace)/((f32)labels.size()));
	f32 hspacepadding = 0.1f/((f32)labels.size());
	IGUIFont* font = env->getSkin()->getFont();
	std::vector<f32> hspaces;
	f32 sumSpaces = 0.f;
	hspaces.reserve(labels.size());
	for(uint32_t i=0; i<labels.size(); i++){
		dimension2d<u32> dim = font->getDimension(labels[i].c_str());
		u32 maxSpace = dim.Width;
		for(uint32_t j=0; j<items.size(); j++){
			dimension2d<u32> dim = font->getDimension(items[j]->fields[i].c_str());
			if(dim.Width>maxSpace){maxSpace = dim.Width;}
		}
		f32 space = Min(maxhspace, ((f32)maxSpace)/lwidth+hspacepadding);
		hspaces.push_back(space);
		sumSpaces += space;
	}
	hspaces[0] += 1.f-iconspace-sumSpaces;//put the remaining space to the first field (usually name), it's guranteed to be >0 because of maxhspace limit
	SColor textColor = env->getSkin()->getColor(EGDC_BUTTON_TEXT);
	//Header:
	fieldButtons.clear();
	AggregateGUIElement* header = new AggregateGUIElement(env, vspaceHeader, 1.f, vspaceHeader, 1.f, false, true, false, {}, {}, false, invisibleAggregationID);
	//header->addSubElement(new EmptyGUIElement(env, iconspace, 1.f, false, false, -1));
	for(uint32_t i=0; i<labels.size(); i++){
		f32 thisSpace = i==0?(hspaces[i]+iconspace):(hspaces[i]);
		BeautifulGUIButton* but;
		if(sortIndex==i){
			f32 arrowSpace = .0325f/thisSpace;
			but = new BeautifulGUIButton(env, thisSpace, 1.f, false, true, false, {
				new BeautifulGUIText(labels[i].c_str(), textColor, 0.f, NULL, true, true, env, .95f-arrowSpace),
				new BeautifulGUIImage(drawer, sortAscending?source->getAscendingIcon():source->getDescendingIcon(), env, arrowSpace, true, -1),
				new EmptyGUIElement(env, 0.05f, 1.f, false, false, -1)
			}, {}, -1);
		}else{
			but = new BeautifulGUIButton(env, thisSpace, 1.f, false, true, false, {
				new BeautifulGUIText(labels[i].c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
			}, {}, -1);
		}
		fieldButtons.push_back(but);
		header->addSubElement(but);
	}
	filesAndHeaderAgg->addSubElement(header);
	filesAgg = new AggregateGUIElement(env, 1.f-vspaceHeader, 1.f, 1.f-vspaceHeader, 1.f, false, false, true, {}, {}, false, invisibleAggregationID);
	filesAndHeaderAgg->addSubElement(filesAgg);
	//Items:
	for(uint32_t i=0; i<items.size(); i++){
		IItemOrganizer::Item* item = items[i];
		AggregateGUIElement* line = new AggregateGUIElement(env, vspace, 1.f, vspace, 1.f, false, true, false, {}, {}, true, listElementAggregationID);
		ITexture* icon = item->isDirectory?source->getFolderIcon():source->getItemIcon(*item);
		line->addSubElement(new BeautifulGUIImage(drawer, icon, env, iconspace, true, -1));
		assert(labels.size()==item->fields.size());
		for(uint32_t j=0; j<item->fields.size(); j++){
			line->addSubElement(new BeautifulGUIText(item->fields[j].c_str(), textColor, 0.f, NULL, j!=0, true, env, hspaces[j]));
		}
		filesAgg->addSubElement(line);
	}
}

void ItemSelectElement::cd(const std::string& path, bool resetPlacesSelection){
	if(organizer->cd(path)){
		itemcbk->OnItemSelect(IItemSelectCallback::DESELECT, NULL, "", this, organizer);
		selectedIndex = -1;
		filesAndHeaderAgg->remove();
		bool createMkdirButton = mkdirBut!=NULL;
		s32 mkdirButtonID = -1;
		if(createMkdirButton){
			mkdirButtonID = mkdirBut->getID();
			mkdirBut->remove();
		}
		pathAgg->remove();
		button2path.clear();
		createNavigationBar(createMkdirButton, mkdirButtonID);
		createFileList();
		if(placesAgg && resetPlacesSelection){//reset selection in places
			int32_t selected = placesAgg->getSingleSelected();
			if(selected>=0){
				placesAgg->getSubElement(selected)->setActive(false);
			}
		}
	}
}

bool ItemSelectElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_GUI_EVENT){
		const SEvent::SGUIEvent& g = event.GUIEvent;
		if(g.EventType==EGET_BUTTON_CLICKED){
			auto it = button2path.find(g.Caller);
			if(it!=button2path.end()){
				std::cout << "cd to: " << it->second << std::endl;
				cd(it->second);
			}else if(g.Caller==mkdirBut){
				new EditBoxDialog(cbk, device, lang->getPhrase(L"ItemSelectElement::MKDIR_DESC", &defaultPhrases).c_str(), lang->getPhrase(L"ItemSelectElement::OK", &defaultPhrases).c_str(), lang->getPhrase(L"ItemSelectElement::CANCEL", &defaultPhrases).c_str());
			}else if(g.Caller==negative){
				itemcbk->OnItemSelect(IItemSelectCallback::CANCEL, NULL, "", this, organizer);
				remove();
				drop();
			}else if(g.Caller==positive){
				if(isSaveDialog){//Save
					std::string filename = convertWStringToUtf8String(nameEdit->getText());
					if(filename.empty()){
						new CMBox(device, lang->getPhrase(L"ItemSelectElement::EMPTY_FILENAME", &defaultPhrases).c_str());
					}else if(organizer->doesExist(filename)){
						overwritebox = new CMBox(device, lang->getPhrase(L"ItemSelectElement::ASK_OVERWRITE", &defaultPhrases).c_str(), .75, .75, lang->getPhrase(L"ItemSelectElement::YES", &defaultPhrases).c_str(), lang->getPhrase(L"ItemSelectElement::NO", &defaultPhrases).c_str());
						addChild(overwritebox);
					}else{
						itemcbk->OnItemSelect(IItemSelectCallback::SAVE, NULL, organizer->getAbsolutePath(filename), this, organizer);
						remove();
						drop();
					}
				}else if(selectedIndex>=0){//Open
					itemcbk->OnItemSelect(IItemSelectCallback::OPEN, items[selectedIndex], organizer->getAbsolutePath(items[selectedIndex]->relativePath), this, organizer);
					remove();
					drop();
				}else{
					new CMBox(device, lang->getPhrase(L"ItemSelectElement::NO_SELECTION", &defaultPhrases).c_str());
				}
			}else{
				for(uint32_t i=0; i<fieldButtons.size(); i++){
					if(g.Caller==fieldButtons[i]){
						if(i==sortIndex){
							sortAscending = !sortAscending;
						}else{
							sortIndex = i;
							sortAscending = true;
						}
						filesAndHeaderAgg->remove();
						createFileList();
						break;
					}
				}
			}
		}else if(g.EventType==EGET_LISTBOX_CHANGED){
			if(g.Caller==placesAgg){
				int32_t selected = placesAgg->getSingleSelected();
				if(selected>=0){
					const std::vector<IItemOrganizer::Place>& p = organizer->getPlaces();
					cd(p[selected].path, false);
				}
			}else if(g.Caller==filesAgg){
				int32_t selected = filesAgg->getSingleSelected();
				selectedIndex = selected;
				if(selected>=0){
					assert(selected<(int32_t)items.size());
					IItemOrganizer::Item* item = items[selected];
					if(item->isDirectory){
						cd(std::string(item->relativePath.c_str()));//create string copy since item pointer will become invalid during cd
					}else{
						if(nameEdit!=NULL){nameEdit->setText(convertUtf8ToWString(item->relativePath).c_str());}
						itemcbk->OnItemSelect(IItemSelectCallback::SELECT, item, organizer->getAbsolutePath(item), this, organizer);
					}
				}else{
					itemcbk->OnItemSelect(IItemSelectCallback::DESELECT, NULL, "", this, organizer);
				}
			}
		}else if(g.EventType==EGET_EDITBOX_CHANGED){
			if(g.Caller==nameEdit){
				int32_t selected = filesAgg->getSingleSelected();
				if(selected>=0){
					filesAgg->getSubElement(selected)->setActive(false);
				}
			}
		}else if(g.EventType==EGET_MESSAGEBOX_YES){
			if(g.Caller==overwritebox){
				std::string filename = convertWStringToUtf8String(nameEdit->getText());
				itemcbk->OnItemSelect(IItemSelectCallback::SAVE, NULL, organizer->getAbsolutePath(filename), this, organizer);
				remove();
				drop();
			}
		}else if(g.EventType==EGET_MESSAGEBOX_NO){
			if(g.Caller==overwritebox){overwritebox = NULL;}
		}
	}
	return IGUIElement::OnEvent(event);
}

void ItemSelectElement::clearSelection(){
	int32_t selected = filesAgg->getSingleSelected();
	if(selected>=0){
		filesAgg->getSubElement(selected)->setActive(false);
		itemcbk->OnItemSelect(IItemSelectCallback::DESELECT, NULL, "", this, organizer);
	}
}

