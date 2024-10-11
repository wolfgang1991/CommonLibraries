#include "CommonIniEditor.h"
#include "StringHelpers.h"
#include "UnicodeCfgParser.h"
#include "utilities.h"
#include "font.h"
#include "IInput.h"
#include "AggregateGUIElement.h"
#include "BeautifulGUIText.h"
#include "BeautifulGUIButton.h"
#include "AggregatableGUIElementAdapter.h"
#include "CMBox.h"
#include "BeautifulCheckBox.h"
#include "mathUtils.h"

#include <IrrlichtDevice.h>
#include <IVideoDriver.h>
#include <IGUIButton.h>
#include <IGUIWindow.h>
#include <IGUIImage.h>
#include <IGUIScrollBar.h>
#include <IGUIFont.h>
#include <IGUIComboBox.h>
#include <IGUIStaticText.h>
#include <IGUIEditBox.h>
#include <IGUIListBox.h>

#include <iostream>

using namespace std;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

CommonIniEditor::CommonIniEditor(ICommonAppContext* context, std::string Section, int FieldCount, std::string* Keys, std::string* FieldNames, std::string* DefaultValues, ValueType* ValTypes, IIniEditorCustomization* customization, const char* HelpFile, IIniEditorPostProcessor* pp):c(context), customization(customization), section(Section), fieldCount(FieldCount), key(Keys), defaultValue(DefaultValues), valType(ValTypes), sel(context, customization->getAlphaBackground(), SColor(255, 255, 255,255)){
	this->pp = pp;
	fieldName = new std::wstring[FieldCount];
	for(int i=0; i<FieldCount; i++){
		fieldName[i] = convertUtf8ToWString(FieldNames[i]);
		if(pp){pp->OnLoadField(key[i], fieldName[i], defaultValue[i]);}
	}
	delete[] FieldNames;
	init(HelpFile);
}

static const std::wstring valueTypeStr[] = {L"INT", L"DOUBLE", L"STRING", L"BOOLEAN", L"NOT_EDITABLE", L"ONE_OF", L"COLOR_RGB", L"COLOR_RGBA"};
static_assert(sizeof(valueTypeStr)/sizeof(valueTypeStr[0])==CommonIniEditor::TYPE_COUNT, "");

static bool areWStringsEqual(const std::wstring& a, const std::wstring& b){//workaround for compiler or valgrind bug (== results in problems)
	if(a.size()!=b.size()){return false;}
	for(uint32_t i=0; i<a.size(); i++){
		if(a[i]!=b[i]){return false;}
	}
	return true;
}

CommonIniEditor::CommonIniEditor(ICommonAppContext* context, std::string Section, const std::wstring& guiCode, IIniEditorCustomization* customization, const char* HelpFile, IIniEditorPostProcessor* pp):c(context), customization(customization), section(Section), sel(context, customization->getAlphaBackground(), SColor(255, 255, 255,255)){
	this->pp = pp;
	UnicodeCfgParser parser(4);
	parser.parse(guiCode);
	const std::list<std::vector<std::wstring>>& result = parser.getResult();
	fieldCount = result.size();
	key = new std::string[fieldCount];
	fieldName = new std::wstring[fieldCount];
	defaultValue = new std::string[fieldCount];
	valType = new ValueType[fieldCount];
	uint32_t i = 0;
	for(auto it = result.begin(); it != result.end(); ++it){
		const std::vector<std::wstring>& line = *it;
		if(line.size()!=4){
			key[i] = "error_stderr";
			fieldName[i] = L"Syntax error";
			valType[i] = NOT_EDITABLE;
			defaultValue[i] = "see stderr";
			std::cerr << "ERROR: Wrong number of arguments (must be 4 for IniEditor)." << std::endl;
			for(const std::wstring& s : *it){std::cerr << convertWStringToUtf8String(s) << " ";}
			std::cerr << std::endl;
		}else{
			key[i] = convertWStringToUtf8String(line[0]);
			fieldName[i] = line[1];
			defaultValue[i] = convertWStringToUtf8String(line[2]);
			if(pp){pp->OnLoadField(key[i], fieldName[i], defaultValue[i]);}
			const std::wstring& t = line[3];
			valType[i] = NOT_EDITABLE;
			for(uint32_t j=0; j<TYPE_COUNT; j++){
				if(areWStringsEqual(valueTypeStr[j],t)){//valueTypeStr[j]==t){
					valType[i] = (ValueType)j;
					break;
				}
			}
		}
		i++;
	}
	init(HelpFile);
}

void CommonIniEditor::init(const char* HelpFile){
	lastSucc = false;
	env = c->getIrrlichtDevice()->getGUIEnvironment();
	driver = env->getVideoDriver();
	int w = driver->getScreenSize().Width; int h = driver->getScreenSize().Height;
	wrect = rect<s32>(0, 0, w, h);
	win = env->addWindow(wrect, false, L"");//Edit
	win->getCloseButton()->setVisible(false); win->setDrawTitlebar(false); win->setDraggable(false);
	cancel = env->addButton(rect<s32>(3*w/14-c->getRecommendedButtonWidth()/2, h-10-c->getRecommendedButtonHeight(), 3*w/14+c->getRecommendedButtonWidth()/2, h-10), win, customization->getCancelButtonId(), L"Cancel");
	showHelpButton = (HelpFile != NULL);
	help = NULL;
	if(showHelpButton){
		help = env->addButton(rect<s32>(7*w/14-c->getRecommendedButtonWidth()/2, h-10-c->getRecommendedButtonHeight(), 7*w/14+c->getRecommendedButtonWidth()/2, h-10), win, -1, L"Help", NULL);
		helpFile = std::string(HelpFile);
	}
	ok = env->addButton(rect<s32>(11*w/14-c->getRecommendedButtonWidth()/2, h-10-c->getRecommendedButtonHeight(), 11*w/14+c->getRecommendedButtonWidth()/2, h-10), win, customization->getOkButtonId(), L"Ok");
	prect = rect<s32>(10, 10, w-10, h-20-c->getRecommendedButtonHeight());
	pw = prect.getWidth()-15;
	ph = prect.getHeight();
	prect.LowerRightCorner.X -= 15;
	agg = NULL;
	input = new IGUIElement*[fieldCount];
	height = new int[fieldCount];
	for(int i=0; i<fieldCount; i++){//Initialwerte setzen
		input[i] = NULL;
	}
	lastContent = new std::wstring[fieldCount];
	win->setVisible(false);
	state = 0;
}

void CommonIniEditor::createGUI(){
	IGUIFont* font = env->getSkin()->getFont();
	f32 labelSpace = 0.5f;
	int wholeWidth = prect.getWidth()*.95f;
	int labelWidth = wholeWidth*labelSpace;
	f32 lineSpace = (f32)c->getRecommendedButtonHeight()/(f32)prect.getHeight();
	f32 emptySpace = 0.5f*lineSpace;
	f32 linesPerPage = (1.f-emptySpace)/(lineSpace+emptySpace);
	f32 scrollSpace = 0.0125f;
	f32 scrollMargin = 0.0125f;
	AggregateGUIElement* content = new AggregateGUIElement(env, 1.f-2.f*(scrollSpace+scrollMargin), 1.f, .95f, 1.f, false, false, true, {}, {}, false, customization->getAggregationID());
	bool useScrollBar = fieldCount>(linesPerPage/2);//if more than half page filled add scrollbar and large empty space below because of the keyboard
	ScrollBar* sb = useScrollBar?new ScrollBar(env, scrollSpace, false, customization->getScrollBarID()):NULL;
	agg = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, true, false, {
		new EmptyGUIElement(env, scrollSpace+scrollMargin, 1.f, false, false, customization->getAggregatableID()),
		content,
		new EmptyGUIElement(env, scrollMargin, 1.f, false, false, customization->getAggregatableID()),
		sb?(IAggregatableGUIElement*)sb:(IAggregatableGUIElement*)new EmptyGUIElement(env, scrollSpace, 1.f, false, false, customization->getAggregatableID())
	}, {}, false, customization->getAggregationID(), NULL, win, prect);
	if(sb){sb->linkToScrollable(content);}
	for(int i=0; i<fieldCount; i++){
		content->addSubElement(new EmptyGUIElement(env, emptySpace, 1.f, false, false, customization->getAggregatableID()));
		IAggregatableGUIElement* finalLine = NULL;
		if(valType[i]==INT || valType[i]==DOUBLE || valType[i]==STRING){
			AggregateGUIElement* line = new AggregateGUIElement(env, lineSpace, 1.f, lineSpace, 1.f, false, true, false, {
				new BeautifulGUIText(makeWordWrappedText(fieldName[i], labelWidth, font).c_str(), 0.f, NULL, false, true, env, labelSpace),
				addAggregatableEditBox(env, L"", 1.f-labelSpace, true, valType[i]==INT?INT_EDIT:(valType[i]==DOUBLE?DOUBLE_EDIT:STRING_EDIT))
			}, {}, false, customization->getInvisibleAggregationID());
			input[i] = getFirstGUIElementChild(*(line->getChildren().begin()+1));
			finalLine = line;
		}else if(valType[i]==NOT_EDITABLE){
			finalLine = new BeautifulGUIText(convertUtf8ToWString(convertWStringToUtf8String(fieldName[i]).append(defaultValue[i])).c_str(), 0.f, NULL, true, true, env, lineSpace);
			input[i] = finalLine;
		}else if(valType[i]==BOOLEAN){
			finalLine = new BeautifulCheckBox(makeWordWrappedText(fieldName[i], 4*wholeWidth/5, font).c_str(), 0.f, NULL, false, true, env, lineSpace);
			input[i] = finalLine;
		}else if(valType[i]==ONE_OF){
			s32 lineCount = 1;
			for(u32 ci = 0; ci<defaultValue[i].size(); ci++){
				if(defaultValue[i][ci]==';'){lineCount++;}
			}
			f32 finalLineSpace = (lineCount>5?5:lineCount)*lineSpace;
			AggregateGUIElement* line = new AggregateGUIElement(env, finalLineSpace, 1.f, finalLineSpace, 1.f, false, true, false, {
				new BeautifulGUIText(makeWordWrappedText(fieldName[i], labelWidth, font).c_str(), 0.f, NULL, false, true, env, labelSpace),
				addAggregatableListBox(env, 1.f-labelSpace)//addAggregatableBeautifulListBox(env, 1.f-labelSpace, customization->getAggregationID(), customization->getInvisibleAggregationID(), customization->getScrollBarID(), customization->getAggregationID(), customization->getAggregatableID())//addAggregatableComboBox(env, 1.f-labelSpace)
			}, {}, false, customization->getInvisibleAggregationID());
			input[i] = getFirstGUIElementChild(*(line->getChildren().begin()+1));
			finalLine = line;
			((IGUIListBox*)input[i])->setItemHeight(c->getRecommendedButtonHeight());//IGUIComboBox
			uint32_t start = 0;
			uint32_t ci = 0;
			for(; ci<defaultValue[i].size(); ci++){
				char c = defaultValue[i][ci];
				if(c==';'){
					((IGUIListBox*)input[i])->addItem(convertUtf8ToWString(defaultValue[i].substr(start,ci-start)).c_str());//IGUIComboBox
					start = ci+1;
				}
			}
			((IGUIListBox*)input[i])->addItem(convertUtf8ToWString(defaultValue[i].substr(start,ci-start)).c_str());//IGUIComboBox
//			if(lineCount>5 && ((IGUIListBox*)input[i])->getVerticalScrollBar()!=NULL){
//				((IGUIListBox*)input[i])->getVerticalScrollBar()->setVisible(false);
//			}
		}else if(valType[i]==COLOR_RGB || valType[i]==COLOR_RGBA){
			f32 buttonSpace = Min(1.f, .1f+(f32)font->getDimension(fieldName[i].c_str()).Width/wholeWidth);
			BeautifulGUIButton* but;
			AggregateGUIElement* line = new AggregateGUIElement(env, lineSpace, 1.f, lineSpace, 1.f, false, true, false, {
				//new EmptyGUIElement(env, (1.f-buttonSpace)/2.f, 1.f, false, false, customization->getAggregatableID()),
				but = new BeautifulGUIButton(env, buttonSpace, 1.f, false, true, false, {
					new BeautifulGUIText(fieldName[i].c_str(), 0.f, NULL, true, true, env, 1.f)
				}, {}, -1),
				new EmptyGUIElement(env, 1.f-buttonSpace, 1.f, false, false, customization->getAggregatableID())
			}, {}, false, customization->getInvisibleAggregationID());
			finalLine = line;
			input[i] = but;
		}
		if(finalLine){content->addSubElement(finalLine);}
	}
	content->addSubElement(new EmptyGUIElement(env, useScrollBar?1.f:emptySpace, 1.f, false, false, customization->getAggregatableID()));
}

void CommonIniEditor::removeGUI(){
	if(agg!=NULL){
		agg->remove();
		agg = NULL;
	}
}

CommonIniEditor::~CommonIniEditor(){
	win->remove();
	delete[] key;
	delete[] fieldName;
	delete[] defaultValue;
	delete[] valType;
	delete[] input;
	delete[] height;
	delete[] lastContent;
	delete pp;
}

void CommonIniEditor::edit(IniFile* Ini){
	createGUI();
	colorState = -1;
	state = 0;
	lastSucc = false;
	ini = Ini;
	win->setVisible(true);
	win->setRelativePosition(wrect);
	env->setFocus(win);
	std::string def;
	for(int i=0; i<fieldCount; i++){
		if(ini->isAvailable(section, key[i])){
			def = ini->get(section, key[i]);
			if(pp){pp->OnLoadValue(key[i], def);}
		}else{
			def = defaultValue[i];
		}
		lastContent[i] = convertUtf8ToWString(def);
		if(valType[i]==INT || valType[i]==DOUBLE || valType[i]==STRING){
			input[i]->setText(lastContent[i].c_str());
		}else if(valType[i]==BOOLEAN){
			((BeautifulCheckBox*)input[i])->setChecked((bool)atoi(def.c_str()));
		}else if(valType[i]==ONE_OF){
			IGUIListBox* ele = (IGUIListBox*)input[i];//IGUIComboBox
			if(ele->getItemCount()>0){ele->setSelected(0);}
			for(uint32_t it=0; it<ele->getItemCount(); it++){
				std::string item = convertWStringToUtf8String(std::wstring(ele->getListItem(it)));//getItem
				if(item.compare(def)==0){
					ele->setSelected(it);
					break;
				}
			}
		}
	}
	bringToFrontRecursive(win);
}

bool CommonIniEditor::isCorrectType(std::wstring content, ValueType type){
	if(type==STRING){
		return true;
	}else if(type==INT){
		std::string c = convertWStringToUtf8String(content);
		if(c.size()>=1){
			if((c[0]<'0' || c[0]>'9') && c[0]!='-'){return false;}
		}
		for(uint32_t i=1; i<c.size(); i++){
			if(c[i]<'0' || c[i]>'9'){return false;}//error state
		}
		return true;
	}else if(type==DOUBLE){
		std::string c = convertWStringToUtf8String(content);
		bool dotRead = false;
		if(c.size()>=1){
			if(c[0]=='.'){
				dotRead = true;
			}else if((c[0]<'0' || c[0]>'9') && c[0]!='-'){
				return false;
			}
		}
		for(uint32_t i=1; i<c.size(); i++){
			if(c[i]=='.'){
				if(dotRead){return false;}
				dotRead = true;
			}else if(c[i]<'0' || c[i]>'9'){
				return false;
			}
		}
		return true;
	}else{
		return false;
	}
}

void CommonIniEditor::render(){
	if(sel.isVisible()){
		sel.render();
	}
}

void CommonIniEditor::processEvent(const irr::SEvent& event){
	win->setRelativePosition(wrect);//fenster soll auf derselben Position bleiben
	//ColorSelector Events
	if(colorState>-1){
		sel.processEvent(event);
		if(!sel.isVisible()){
			if(sel.lastSuccess()){
				SColor color = sel.getColor();
				std::string keyname = key[colorState]; keyname.append(".R");
				ini->set(section, keyname, convertToString(color.getRed()));
				keyname = key[colorState]; keyname.append(".G");
				ini->set(section, keyname, convertToString(color.getGreen()));
				keyname = key[colorState]; keyname.append(".B");
				ini->set(section, keyname, convertToString(color.getBlue()));
				keyname = key[colorState]; keyname.append(".A");
				ini->set(section, keyname, convertToString(color.getAlpha()));
			}
			for(int i=0; i<fieldCount; i++){
				if(input[i]){input[i]->setEnabled(true);}
			}
			colorState = -1;
		}
	}else{//own Events
		if(event.EventType == EET_GUI_EVENT){
			const irr::SEvent::SGUIEvent& guievent = event.GUIEvent;
			if(guievent.EventType == EGET_EDITBOX_CHANGED){
				for(int i=0; i<fieldCount; i++){
					//Type Checking:
					if(input[i] && (guievent.Caller==input[i]) && (valType[i]==INT || valType[i]==DOUBLE || valType[i]==STRING)){
						std::wstring thisContent = std::wstring(input[i]->getText());
						if(!isCorrectType(thisContent, valType[i])){
							input[i]->setText(lastContent[i].c_str());
						}else{
							lastContent[i] = thisContent;
						}
					}
				}
			}else if(guievent.EventType == EGET_BUTTON_CLICKED){
				for(int i=0; i<fieldCount; i++){
					if((valType[i]==COLOR_RGB || valType[i]==COLOR_RGBA) && guievent.Caller==input[i]){
						colorState = i;
						std::string def[4]; int di=0;
						for(uint32_t ci=0; ci<defaultValue[i].size(); ci++){
							if(di<4 && defaultValue[i][ci] == ','){
								di++;
							}else{
								def[di].append(std::string(&(defaultValue[i][ci]),1));
							}
						}
						std::string keyname = key[i]; keyname.append(".R");
						if(ini->isAvailable(section, keyname)){
							def[0] = ini->get(section, keyname);
						}
						keyname = key[i]; keyname.append(".G");
						if(ini->isAvailable(section, keyname)){
							def[1] = ini->get(section, keyname);
						}
						keyname = key[i]; keyname.append(".B");
						if(ini->isAvailable(section, keyname)){
							def[2] = ini->get(section, keyname);
						}
						keyname = key[i]; keyname.append(".A");
						if(ini->isAvailable(section, keyname)){
							def[3] = ini->get(section, keyname);
						}
						sel.setColor(SColor(atoi(def[3].c_str()), atoi(def[0].c_str()), atoi(def[1].c_str()), atoi(def[2].c_str())));
						sel.select(valType[i]==COLOR_RGBA);
						colorState = i;
						for(int i=0; i<fieldCount; i++){
							if(input[i]){input[i]->setEnabled(false);}
						}
						break;
					}
				}
				if(guievent.Caller==ok){
					lastSucc = true;
					for(int i=0; i<fieldCount; i++){
						std::string value;
						if(valType[i]==INT || valType[i]==DOUBLE || valType[i]==STRING){
							value = convertWStringToUtf8String(std::wstring(input[i]->getText()));
						}else if(valType[i]==BOOLEAN){
							value = convertToString(((BeautifulCheckBox*)input[i])->isChecked());
						}else if(valType[i]==NOT_EDITABLE){
							value = defaultValue[i];
						}else if(valType[i]==ONE_OF){
							int idx = ((IGUIListBox*)input[i])->getSelected();//IGUIComboBox
							if(idx==-1){idx=0;}
							value = convertWStringToUtf8String(std::wstring(((IGUIListBox*)input[i])->getListItem(idx)));//IGUIComboBox getItem
						}else if(valType[i]==COLOR_RGB || valType[i]==COLOR_RGBA){
							std::string def[4]; int di=0;
							for(uint32_t ci=0; ci<defaultValue[i].size(); ci++){
								if(di<4 && defaultValue[i][ci] == ','){
									di++;
								}else{
									def[di].append(std::string(&(defaultValue[i][ci]),1));
								}
							}
							std::string keyname = key[i]; keyname.append(".R");
							if(!ini->isAvailable(section, keyname)){
								ini->set(section, keyname, def[0]);
							}
							keyname = key[i]; keyname.append(".G");
							if(!ini->isAvailable(section, keyname)){
								ini->set(section, keyname, def[1]);
							}
							keyname = key[i]; keyname.append(".B");
							if(!ini->isAvailable(section, keyname)){
								ini->set(section, keyname, def[2]);
							}
							if(valType[i]==COLOR_RGBA){
								keyname = key[i]; keyname.append(".A");
								if(!ini->isAvailable(section, keyname)){
									ini->set(section, keyname, def[3]);
								}
							}
						}
						if(valType[i]!=COLOR_RGB && valType[i]!=COLOR_RGBA){
							if(pp){pp->OnSaveValue(key[i], value);}
							ini->set(section, key[i], value);
						}
					}
					win->setVisible(false);
					removeGUI();
					OnSuccess();
				}else if(guievent.Caller==cancel){
					cancelEdit();
				}else if(guievent.Caller==help){
					customization->OnHelpButtonPressed(helpFile.c_str());
				}
			}
		}
	}
}

void CommonIniEditor::setIniPostProcessor(IIniEditorPostProcessor* pp){
	delete this->pp;
	this->pp = pp;
}

void CommonIniEditor::cancelEdit(){
	win->setVisible(false);
	removeGUI();
	OnCancel();
}

bool CommonIniEditor::isVisible(){
	return win->isVisible();
}

IniEditorGUIElement::IniEditorGUIElement(CommonIniEditor* iniEditor, irr::gui::IGUIElement* parent, bool removeOnScreenResize):IGUIElement(EGUIET_ELEMENT, iniEditor->getGUIEnvironment(), parent, -1, rect<s32>(0,0,iniEditor->getVideoDriver()->getScreenSize().Width,iniEditor->getVideoDriver()->getScreenSize().Height)){
	onSuccess = [](){};
	this->removeOnScreenResize = removeOnScreenResize;
	this->iniEditor = iniEditor;
	mustRemove = false;
	if(parent==NULL){
		iniEditor->getGUIEnvironment()->getRootGUIElement()->addChild(this);
	}
	addChild(iniEditor->getWindow());
	ss = Environment->getVideoDriver()->getScreenSize();
	drop();
}

IniEditorGUIElement::~IniEditorGUIElement(){
	if(iniEditor->isVisible()){//if not cancelled already
		iniEditor->cancelEdit();
	}
	delete iniEditor;
}
	
bool IniEditorGUIElement::OnEvent(const SEvent& event){
	if(mustRemove){
		remove();
		return true;
	}else{
		grab();
		bool res = IGUIElement::OnEvent(event);//it may get deleted in onSuccess
		if(iniEditor->isVisible()){
			iniEditor->processEvent(event);
			if(iniEditor->lastSuccess()){
				onSuccess();
			}else if(!iniEditor->isVisible()){
				onCancel();
			}
		}
		drop();
		return res;
	}
}

void IniEditorGUIElement::draw(){
	setVisible(iniEditor->isVisible());
	if(isVisible()){
		IGUIElement::draw();
		iniEditor->render();
		if(removeOnScreenResize){
			dimension2d<u32> newSS = Environment->getVideoDriver()->getScreenSize();
			if(ss!=newSS){
				ss = newSS;
				setVisible(false);
				mustRemove = true;
			}
		}
	}else{
		mustRemove = true;
	}
}

void IniEditorGUIElement::edit(IniFile* ini, const std::function<void()>& onSuccess, const std::function<void()>& onCancel){
	setVisible(true);
	this->onSuccess = onSuccess;
	this->onCancel = onCancel;
	iniEditor->edit(ini);
}

void IniEditorGUIElement::setVisible(bool visible){
	iniEditor->getWindow()->setVisible(visible);
	IGUIElement::setVisible(visible);
}
