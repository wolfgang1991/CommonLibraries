#include <ChooseFromListDialog.h>

#include <AggregateGUIElement.h>
#include <AggregatableGUIElementAdapter.h>
#include <timing.h>

#include <IrrlichtDevice.h>
#include <IVideoDriver.h>
#include <IGUIEnvironment.h>
#include <IGUIFont.h>
#include <IGUIWindow.h>
#include <IGUIStaticText.h>
#include <IGUIButton.h>

using namespace irr;
using namespace video;
using namespace gui;
using namespace core;

OutsideCancelWindow::OutsideCancelWindow(irr::gui::IGUIEnvironment* environment, irr::gui::IGUIWindow* win, irr::s32 id):
	IGUIWindow(environment, environment->getRootGUIElement(), id, rect<s32>(0,0,environment->getVideoDriver()->getScreenSize().Width,environment->getVideoDriver()->getScreenSize().Height)){
	this->win = win;
	addChild(win);
}

irr::gui::IGUIButton* OutsideCancelWindow::getCloseButton() const{return win->getCloseButton();}

irr::gui::IGUIButton* OutsideCancelWindow::getMinimizeButton() const{return win->getMinimizeButton();}

irr::gui::IGUIButton* OutsideCancelWindow::getMaximizeButton() const{return win->getMaximizeButton();}

bool OutsideCancelWindow::isDraggable() const{return win->isDraggable();}

void OutsideCancelWindow::setDraggable(bool draggable){return win->setDraggable(draggable);}

void OutsideCancelWindow::setDrawBackground(bool draw){return win->setDrawBackground(draw);}

bool OutsideCancelWindow::getDrawBackground() const{return win->getDrawBackground();}

void OutsideCancelWindow::setDrawTitlebar(bool draw){return win->setDrawTitlebar(draw);}

bool OutsideCancelWindow::getDrawTitlebar() const{return win->getDrawTitlebar();}

irr::core::rect<s32> OutsideCancelWindow::getClientRect() const{return win->getClientRect();}

bool OutsideCancelWindow::OnEvent(const SEvent& event){
	if(event.EventType==EET_MOUSE_INPUT_EVENT){
		const SEvent::SMouseInput& m = event.MouseInput;
		if(m.Event==EMIE_LMOUSE_LEFT_UP){
			vector2d<s32> mPos(m.X, m.Y);
			if(!win->getAbsolutePosition().isPointInside(mPos)){
				SEvent toPost;
				toPost.EventType = EET_GUI_EVENT;
				toPost.GUIEvent.Caller = this;
				toPost.GUIEvent.Element = NULL;
				toPost.GUIEvent.EventType = EGET_MESSAGEBOX_NO;
				Parent->OnEvent(toPost);
				remove();
				drop();
				return true;
			}
		}
	}
	return IGUIWindow::OnEvent(event);
}

irr::gui::IGUIWindow* createChooseFromListDialog(irr::IrrlichtDevice* device, std::vector<irr::gui::IGUIButton*>& buttonsOut, const std::vector<int32_t>& buttonIds, const std::vector<std::wstring>& buttonLabels, const std::wstring& headline, int32_t aggregationId, int32_t aggregatableId, bool modal, bool cancelByClickOutside){
	IVideoDriver* driver = device->getVideoDriver();
	IGUIEnvironment* env = device->getGUIEnvironment();
	dimension2d<u32> dim = driver->getScreenSize();
	u32 w = dim.Width, h = dim.Height;
	double sqrtArea = sqrt(w*h);
	int32_t buttonHeight = (int32_t)(0.0625*0.79057*sqrtArea+.5);
	s32 padding = (s32)(.01*sqrtArea+.5);
	IGUIFont* font = env->getSkin()->getFont();
	u32 sizeW = 4*padding+font->getDimension(headline.c_str()).Width;
	for(uint32_t i=0; i<buttonLabels.size(); i++){
		u32 reqSizeW = 4*padding+font->getDimension(buttonLabels[i].c_str()).Width;
		if(reqSizeW>sizeW){sizeW = reqSizeW;}
	}
	u32 sizeH = 3*padding+1.2*buttonHeight+(buttonLabels.size()*1.2*buttonHeight);
	IGUIWindow* win = env->addWindow(rect<s32>(w/2-sizeW/2, h/2-sizeH/2, w/2+sizeW/2, h/2+sizeH/2), modal);
	win->getCloseButton()->setVisible(false);
	win->setDraggable(false);
	win->setDrawTitlebar(false);
	IGUIStaticText* headlineEle = env->addStaticText(headline.c_str(), rect<s32>(padding,padding,sizeW-padding,padding+buttonHeight), false, false, win);
	headlineEle->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	rect<s32> content(padding, 2*padding+buttonHeight, sizeW-padding, sizeH-padding);
	f32 lineYPart = ((f32)buttonHeight)/(f32)content.getHeight();
	AggregateGUIElement* buttonList = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, false, true, {}, {}, false, aggregationId, NULL, win, content);
	buttonList->addSubElement(new EmptyGUIElement(env, .2f*lineYPart, 1.f/1.f, false, false, aggregatableId));
	buttonsOut.resize(buttonLabels.size());
	for(uint32_t i=0; i<buttonLabels.size(); i++){
		buttonsOut[i] = env->addButton(rect<s32>(0,0,0,0), NULL, buttonIds[i], buttonLabels[i].c_str());
		buttonList->addSubElement(new AggregatableGUIElementAdapter(env, lineYPart, 4.f/1.f, false, buttonsOut[i], false, aggregatableId));
		buttonList->addSubElement(new EmptyGUIElement(env, .2f*lineYPart, 1.f/1.f, false, false, aggregatableId));
	}
	if(cancelByClickOutside){
		win = new OutsideCancelWindow(env, win);
	}
	return win;
}

class ChooseFromListEventReceiver : public irr::IEventReceiver{
	
	public:
	
	uint32_t res;
	bool running;
	std::vector<irr::gui::IGUIButton*> buttons;
	
	ChooseFromListEventReceiver():res(0),running(true){}
	
	bool OnEvent(const SEvent& event){
		if(event.EventType==EET_GUI_EVENT){
			const SEvent::SGUIEvent& g = event.GUIEvent;
			if(g.EventType==EGET_BUTTON_CLICKED){
				for(uint32_t i=0; i<buttons.size(); i++){
					if(g.Caller==buttons[i]){
						res = i;
						running = false;
						break;
					}
				}
			}
		}
		return false;
	}
	
};

uint32_t startChooseFromListDialog(irr::IrrlichtDevice* device, const std::vector<int32_t>& buttonIds, const std::vector<std::wstring>& buttonLabels, const std::wstring& headline, int32_t aggregationId, int32_t aggregatableId, irr::video::SColor bgColor, bool modal){
	IVideoDriver* driver = device->getVideoDriver();
	IGUIEnvironment* env = device->getGUIEnvironment();
	IEventReceiver* oldrcv = device->getEventReceiver();
	ChooseFromListEventReceiver rcv;
	device->setEventReceiver(&rcv);
	irr::core::dimension2d<irr::u32> screenSize = driver->getScreenSize();
	double screenChangeTime = getSecs();
	bool screenChanged = false;
	irr::gui::IGUIWindow* win = createChooseFromListDialog(device, rcv.buttons, buttonIds, buttonLabels, headline, aggregationId, aggregatableId, modal);
	win->getParent()->sendToBack(win);
	while(device->run() && rcv.running){
		driver->beginScene(true, true, bgColor);
		env->drawAll();
		driver->endScene();
		device->yield();
		if(screenChanged && getSecs()-screenChangeTime>.33){
			win->remove();
			win = createChooseFromListDialog(device, rcv.buttons, buttonIds, buttonLabels, headline, aggregationId, aggregatableId, modal);
			win->getParent()->sendToBack(win);
			screenChanged = false;
		}
		if(driver->getScreenSize()!=screenSize){
			screenSize = driver->getScreenSize();
			screenChanged = true;
			screenChangeTime = getSecs();
		}
	}
	win->remove();
	device->setEventReceiver(oldrcv);
	return rcv.res;
}
