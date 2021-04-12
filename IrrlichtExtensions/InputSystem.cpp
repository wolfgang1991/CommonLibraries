#include <timing.h>
#include <InputSystem.h>
#include <ICommonAppContext.h>
#include <KeyInput.h>
#include <TouchKeyboard.h>
#include <StringHelpers.h>
#include <IniFile.h>
#include <utilities.h>

#include <IrrlichtDevice.h>
#include <IrrCompileConfig.h>
#include <IGUIEnvironment.h>
#include <IGUIElement.h>

#include <string>
#include <map>

using namespace std;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

InvalidIntDetector::InvalidIntDetector(){
	reset();
}

bool InvalidIntDetector::nextChar(char c){
	bool res = (c>='0' && c<='9') || (firstChar && c=='-');
	firstChar = false;
	return res;
}

void InvalidIntDetector::reset(){
	firstChar = true;
}


InvalidDoubleDetector::InvalidDoubleDetector(){
	reset();
}

bool InvalidDoubleDetector::nextChar(char c){
	if(z==0){
		if((c>='0' && c<='9') || c=='-'){
			z = 1;
			return true;
		}else if(c=='.'){
			z = 2;
			return true;
		}else{
			return false;
		}
	}else if(z==1){
		if(c=='.'){
			z = 2;
			return true;
		}else if(c>='0' && c<='9'){
			return true;
		}else{
			return false;
		}
	}else if(z==2){
		if(c>='0' && c<='9'){
			return true;
		}else{
			return false;
		}
	}
	return true;
}

void InvalidDoubleDetector::reset(){
	z = 0;
}


InputSystem::InputSystem(ICommonAppContext* context, const std::vector<KeyboardDefinition>& keyboards, const std::string& changeTexturePath, const std::vector<IInput*>& additionalInputs, bool preferPlatformDependentInput):c(context){
	device = c->getIrrlichtDevice();
	env = device->getGUIEnvironment();
	driver = device->getVideoDriver();
	mouseDown = false;
	inputColor = SColor(160, 0, 255, 0);
	button = new TouchKey(c, 0, 0, irr::KEY_TAB, irr::core::rect<s32>(0,0,1.5f*c->getRecommendedButtonWidth(), 1.5f*c->getRecommendedButtonHeight()), driver->getTexture(c->getPath(changeTexturePath).c_str()));
	inputVisible = false;
	activeInput = keyboards.empty()?0:1;//first touch keyboard
	input.push_back(new KeyInput(c, preferPlatformDependentInput));
	assert(input.size()==1);//KeyInput must be the first!
	for(uint32_t i=0; i<keyboards.size(); i++){
		input.push_back(new TouchKeyboard(c, inputColor, keyboards[i]));
	}
	input.insert(input.end(), additionalInputs.begin(), additionalInputs.end());
	dfa[INT_EDIT] = new InvalidIntDetector();
	dfa[DOUBLE_EDIT] = new InvalidDoubleDetector();
	editState = 0;
	setColor(SColor(255,255,255,255));
	c->getEventReceiver()->addSubEventReceiver(this);
}

void InputSystem::setPlatformDependentInputPreferred(bool prefer){
	((KeyInput*)input[0])->setPreferred(prefer);
	if(prefer){
		activeInput = 0;
	}
}

bool InputSystem::isPlatformDependentInputPreferred() const{
	return ((KeyInput*)input[0])->isPreferred();
}

bool InputSystem::OnEvent(const irr::SEvent& event){
	return !processEvent(event);
}

void InputSystem::loadInputSettingsFromIni(IniFile* ini){
	setColor(SColor(atoi(ini->get("","inputColor.A").c_str()), atoi(ini->get("","inputColor.R").c_str()), atoi(ini->get("","inputColor.G").c_str()), atoi(ini->get("","inputColor.B").c_str())));
}

void InputSystem::setColor(irr::video::SColor Color){
	inputColor = Color;
	for(uint32_t i=0; i<input.size(); i++){
		input[i]->setColor(Color);
	}
}

InputSystem::~InputSystem(){
	if(inputVisible){input[activeInput]->OnDeSelect();}
	c->getEventReceiver()->removeSubEventReceiver(this);
	delete button;
	for(uint32_t i=0; i<input.size(); i++){
		delete input[i];
	}
	for(std::map<int, IInvalidCharDetector*>::iterator it = dfa.begin(); it != dfa.end(); ++it){
		delete it->second;
	}
}

void InputSystem::handleEditBoxMarking(){
	IGUIElement* focus = env->getFocus();
	if(focus){
		s32 id = focus->getID();
		if(focus->getType()==EGUIET_EDIT_BOX && (id == DESTINATION_EDIT || id == STRING_EDIT || id == INT_EDIT || id == DOUBLE_EDIT)){
			SEvent post;
			post.EventType = EET_KEY_INPUT_EVENT;
			post.KeyInput.Shift = false;
			post.KeyInput.Control = false;
			post.KeyInput.Char = 0;
			post.KeyInput.Key = KEY_END;
			post.KeyInput.PressedDown = true;
			device->postEventFromUser(post);//end pressed
			post.KeyInput.PressedDown = false;
			device->postEventFromUser(post);//end released
			if(id==DESTINATION_EDIT || id==INT_EDIT || id==DOUBLE_EDIT){
				post.EventType = EET_KEY_INPUT_EVENT;
				post.KeyInput.Char = L'a';
				post.KeyInput.Control = true;
				post.KeyInput.Key = KEY_KEY_A;
				post.KeyInput.Shift = false;
				post.KeyInput.PressedDown = true;
				device->postEventFromUser(post);//Ctrl+A pressed
				post.KeyInput.PressedDown = false;
				device->postEventFromUser(post);//Ctrl+A released
			}
		}
	}
}

bool InputSystem::processEvent(const irr::SEvent& event){
	bool ret = true;
	if(inputVisible){
		ret = ret && input[activeInput]->processEvent(event);
	}
	if(ret){
		if(event.EventType == EET_MOUSE_INPUT_EVENT){//Change of Input Methods
			mouseDown = event.MouseInput.isLeftPressed() && event.MouseInput.Event!=EMIE_LMOUSE_LEFT_UP;
			if(inputVisible){
				bool mInside = false;
				if(button->processMouseEvent(event.MouseInput, &mInside)){
					if(!button->getEvent().PressedDown){//released
						input[activeInput]->OnDeSelect();
						activeInput = (activeInput+1)%input.size();
						input[activeInput]->OnSelect();
					}
				}
				ret = ret && (!mInside);
			}
			if(ret){
				if(event.MouseInput.Event==EMIE_LMOUSE_LEFT_UP && editState==1){//if editbox focussed and mouse released
					editState = 2;
					time = getSecs();
				}
			}
		}else if(event.EventType == EET_GUI_EVENT){
			const SEvent::SGUIEvent& gevent = event.GUIEvent;
			if(gevent.EventType==EGET_EDITBOX_CHANGED){//Remove invalid chars depending on the DFAs
				std::map<int, IInvalidCharDetector*>::iterator it = dfa.find(gevent.Caller->getID());
				if(it != dfa.end()){
					it->second->reset();
					std::string txt = convertWStringToUtf8String(gevent.Caller->getText());
					std::stringstream ss;
					for(uint32_t i=0; i<txt.size(); i++){
						if(it->second->nextChar(txt[i])){
							ss << txt[i];
						}
					}
					gevent.Caller->setText(convertUtf8ToWString(ss.str()).c_str());
				}
			}else if(gevent.EventType==EGET_EDITBOX_ENTER){
				s32 id = gevent.Caller->getID();
				if(id!=MULTILINE_STRING_EDIT && id!=TERMINAL_EDIT){
					env->setFocus(NULL);
				}
			}else if(gevent.EventType==EGET_ELEMENT_FOCUSED){
				IGUIElement* newFocussed = gevent.Caller;
				if(newFocussed){
					if(newFocussed->getType()==EGUIET_EDIT_BOX){editState = 1;}
				}
			}
		}
	}
	return ret;
}

void InputSystem::render(){
	bool oldInputVis = inputVisible;
	IGUIElement* ele = env->getFocus();
	if(ele){//Make inputs visible if editbox focussed
		if((ele->getType() == EGUIET_EDIT_BOX) && (ele->getID()!=NO_EDIT) && (!mouseDown || inputVisible) && ele->isTrulyVisible()){
			if(!inputVisible){//select preferred input on first time
				irr::s32 id = ele->getID();
				if(!input[activeInput]->isPreferredInput(id)){
					for(uint32_t i=0; i<input.size(); i++){
						if(input[i]->isPreferredInput(id)){
							activeInput = i;
							break;
						}
					}
				}
			}
			inputVisible = true;
		}else{
			inputVisible = false;
		}
	}else{
		inputVisible = false;
	}
	if(oldInputVis && !inputVisible){
		input[activeInput]->OnDeSelect();
	}else if(!oldInputVis && inputVisible){
		input[activeInput]->OnSelect();
	}
	if(inputVisible){
		button->render(c->getFlexibleFont(), true, inputColor);
		input[activeInput]->render();
	}
	//editState delay
	if(editState==2){
		if(getSecs()-time>0.25){
			editState = 0;
			handleEditBoxMarking();
		}
	}
}

void InputSystem::setChangeButtonLocation(const irr::core::rect<irr::s32>& rectangle){
	button->setRectangle(rectangle);
}
