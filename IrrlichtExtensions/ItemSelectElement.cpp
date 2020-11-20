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

static void printItemVector(const std::vector<IItemOrganizer::Item*>& v){
	for(uint32_t i=0; i<v.size(); i++){
		IItemOrganizer::Item* item = v[i];
		for(uint32_t j=0; j<item->fields.size(); j++){
			std::cout << "field[" << j << "] = \"" << convertWStringToUtf8String(item->fields[j]) << "\"" << std::endl;
		}
	}
}

static const ConstantLanguagePhrases defaultPhrases({
	{L"ItemSelectElement::NAME",L"Name:"},
	{L"ItemSelectElement::PLACES",L"Places:"},
	{L"ItemSelectElement::CANCEL",L"Cancel"},
	{L"ItemSelectElement::OPEN",L"Open"},
	{L"ItemSelectElement::SAVE",L"Save"},
	{L"ItemSelectElement::MKDIR_ERROR",L"Error while creating new directory."},
	{L"ItemSelectElement::MKDIR_DESC",L"Please enter the name of the new directory:"},
	{L"ItemSelectElement::OK",L"OK"}
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

ItemSelectElement::ItemSelectElement(irr::IrrlichtDevice* device, Drawer2D* drawer, const std::wstring& defaultSaveFileName, IItemOrganizer* organizer, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 mkdirButtonID, bool isSaveDialog, IItemSelectIconSource* source, const std::initializer_list<IAggregatableGUIElement*>& subElements, irr::f32 w, irr::f32 h, ILanguagePhrases* phrases, bool modal):
	IGUIElement(irr::gui::EGUIET_ELEMENT, device->getGUIEnvironment(), NULL, -1, modal?rect<s32>(0,0,device->getVideoDriver()->getScreenSize().Width,device->getVideoDriver()->getScreenSize().Height):rect<s32>(0,0,0,0)){
	this->device = device;
	this->organizer = organizer;
	this->isSaveDialog = isSaveDialog;
	this->aggregationID = aggregationID;
	this->source = source;
	this->listElementAggregationID = listElementAggregationID;
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
	buttonHeight = (s32)(0.0625*0.79057*sqrtArea);
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
	}else{
		nameEdit = NULL;
	}
	parent = win;
	createPlacesGUI(rect<s32>(padding, padding/2+startY, pw/4-padding, ph-3*padding/2-buttonHeight), padding/2, win, aggregationID, listElementAggregationID, buttonHeight);
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
		new BeautifulGUIButton(env, 0.13f, 1.f, false, true, false,{
			new BeautifulGUIText(lang->getPhrase(L"ItemSelectElement::CANCEL", &defaultPhrases).c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, -1),
		new EmptyGUIElement(env, 0.04f, 1.f, false, false, -1),
		new BeautifulGUIButton(env, 0.13f, 1.f, false, true, false,{
			new BeautifulGUIText(isSaveDialog?(lang->getPhrase(L"ItemSelectElement::SAVE", &defaultPhrases).c_str()):(lang->getPhrase(L"ItemSelectElement::OPEN", &defaultPhrases).c_str()), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, -1)
	}, {}, false, listElementAggregationID, NULL, win, rect<s32>(padding, ph-padding-buttonHeight, pw-padding, ph-padding));
	printItemVector(organizer->ls(1));
	
}

ItemSelectElement::~ItemSelectElement(){
	delete cbk;
}

void ItemSelectElement::createPlacesGUI(const irr::core::rect<irr::s32>& r, irr::s32 padding, irr::gui::IGUIElement* parent, irr::s32 aggregationID, irr::s32 listElementAggregationID, irr::s32 buttonHeight){
	IGUIFont* font = env->getSkin()->getFont();
	const std::wstring& placesLabel = lang->getPhrase(L"ItemSelectElement::PLACES", &defaultPhrases);
	dimension2d<u32> placesLabelSize = font->getDimension(placesLabel.c_str());
	SColor textColor = env->getSkin()->getColor(EGDC_BUTTON_TEXT);
	new BeautifulGUIText(placesLabel.c_str(), textColor, 0.f, NULL, true, true, env, 1.f, -1, 1.f, NULL, parent, rect<s32>(r.UpperLeftCorner.X, r.UpperLeftCorner.Y+padding, r.LowerRightCorner.X, r.UpperLeftCorner.Y+placesLabelSize.Height+2*padding));
	AggregateGUIElement* list = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, false, true, {}, {}, false, aggregationID, NULL, parent, rect<s32>(r.UpperLeftCorner.X, r.UpperLeftCorner.Y+placesLabelSize.Height+3*padding, r.LowerRightCorner.X, r.LowerRightCorner.Y));
	const std::vector<IItemOrganizer::Place>& p = organizer->getPlaces();
	f32 space = 1.5f*((f32)buttonHeight)/((f32)r.getHeight());
	for(uint32_t i=0; i<p.size(); i++){
		list->addSubElement(new AggregateGUIElement(env, space, 1.f, space, 1.f, false, true, false, {
			new BeautifulGUIText(p[i].indication.c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
		}, {}, true, listElementAggregationID));
	}
}

void ItemSelectElement::createNavigationBar(bool createMkdirButton, irr::s32 mkdirButtonID){
	pathAgg = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, true, true, {}, {}, false, aggregationID, NULL, parent, createMkdirButton?rect<s32>(navBarRect.UpperLeftCorner.X, navBarRect.UpperLeftCorner.Y, navBarRect.LowerRightCorner.X-3*navBarRect.getHeight()/2, navBarRect.LowerRightCorner.Y):navBarRect);
	if(createMkdirButton){
		mkdirBut = env->addButton(rect<s32>(navBarRect.LowerRightCorner.X-navBarRect.getHeight(), navBarRect.UpperLeftCorner.Y, navBarRect.LowerRightCorner.X, navBarRect.LowerRightCorner.Y), parent, mkdirButtonID);
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
	irr::f32 vspaceHeader = 1.25f*((f32)buttonHeight)/((f32)fileListRect.getHeight());
	irr::f32 vspace = 1.25f*((f32)buttonHeight)/((f32)fileListRect.getHeight()-buttonHeight);
	irr::f32 hspace = 1.f/((f32)labels.size());//TODO individual weight for fields (e.g. filesize needs much less space than name)
	SColor textColor = env->getSkin()->getColor(EGDC_BUTTON_TEXT);
	//Header
	fieldButtons.clear();
	AggregateGUIElement* header = new AggregateGUIElement(env, vspaceHeader, 1.f, vspaceHeader, 1.f, false, true, false, {}, {}, false, listElementAggregationID);
	for(uint32_t i=0; i<labels.size(); i++){
		BeautifulGUIButton* but;
		if(sortIndex==i){
			but = new BeautifulGUIButton(env, hspace, 1.f, false, true, false, {
				new BeautifulGUIText(labels[i].c_str(), textColor, 0.f, NULL, true, true, env, .85f),
				new BeautifulGUIImage(drawer, sortAscending?source->getAscendingIcon():source->getDescendingIcon(), env, .1f, true, -1),
				new EmptyGUIElement(env, 0.05f, 1.f, false, false, -1)
			}, {}, -1);
		}else{
			but = new BeautifulGUIButton(env, hspace, 1.f, false, true, false, {
				new BeautifulGUIText(labels[i].c_str(), textColor, 0.f, NULL, true, true, env, 1.f)
			}, {}, -1);
		}
		fieldButtons.push_back(but);
		header->addSubElement(but);
	}
	filesAndHeaderAgg->addSubElement(header);
	filesAgg = new AggregateGUIElement(env, 1.f-vspaceHeader, 1.f, 1.f-vspaceHeader, 1.f, false, false, true, {}, {}, false, listElementAggregationID);
	filesAndHeaderAgg->addSubElement(filesAgg);
	//Items
	std::vector<IItemOrganizer::Item*> items = organizer->ls(sortIndex);
	if(!sortAscending){std::reverse(std::begin(items), std::end(items));}
	for(uint32_t i=0; i<items.size(); i++){
		IItemOrganizer::Item* item = items[i];
		AggregateGUIElement* line = new AggregateGUIElement(env, vspace, 1.f, vspace, 1.f, false, true, false, {}, {}, true, listElementAggregationID);
		for(uint32_t j=0; j<item->fields.size(); j++){
			//TODO add icon for first field
			line->addSubElement(new BeautifulGUIText(item->fields[j].c_str(), textColor, 0.f, NULL, j!=0, true, env, hspace));
		}
		filesAgg->addSubElement(line);
	}
}

void ItemSelectElement::cd(const std::string& path){
	if(organizer->cd(path)){
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
				//TODO
			}else if(g.Caller==positive){
				//TODO
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
		}
	}
	return IGUIElement::OnEvent(event);
}

void ItemSelectElement::OnPostRender(irr::u32 timeMs){
	IGUIElement::OnPostRender(timeMs);
	//std::cout << "pos: " << pathAgg->getScrollPosition() << std::endl;
}

