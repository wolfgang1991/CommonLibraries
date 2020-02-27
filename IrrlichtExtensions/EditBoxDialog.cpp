#include <EditBoxDialog.h>
#include <GUI.h>
#include <IniFile.h>

#include <IVideoDriver.h>
#include <IGUIWindow.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IrrlichtDevice.h>
#include <IGUIEditBox.h>
#include <IrrCompileConfig.h>

using namespace irr;
using namespace core;
using namespace gui;
using namespace video;

static const char* guiCodeString = 
"EleCount = 4\n"
"Ratio = 1.77778\n"
"\n"
"[E0]\n"
"HAlign = Center\n"
"Id = ttitle\n"
"Rect = 0.0625,0.0625,0.9375,0.3125\n"
"Text = Title\n"
"Type = Text\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n"
"\n"
"[E1]\n"
"Default = \n"
"EditType = STRING_EDIT\n"
"HAlign = Left\n"
"Id = eedit\n"
"Rect = 0.0625,0.4375,0.9375,0.625\n"
"Text = \n"
"Type = EditBox\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n"
"\n"
"[E2]\n"
"HAlign = Center\n"
"Id = bok\n"
"Rect = 0.625,0.75,0.9375,0.9375\n"
"Text = Ok\n"
"Type = Button\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n"
"\n"
"[E3]\n"
"HAlign = Center\n"
"Id = bcancel\n"
"Rect = 0.0625,0.75,0.375,0.9375\n"
"Text = Cancel\n"
"Type = Button\n"
"VAlign = Center\n"
"VisibilityDomain = 0\n";

EditBoxDialog::EditBoxDialog(IEditBoxDialogCallback* cbk, irr::IrrlichtDevice* device, const wchar_t* title, const wchar_t* positiveLabel, const wchar_t* negativeLabel, const wchar_t* defaultText, bool isModal, bool isPasswordBox, wchar_t passwordChar):
	IGUIElement(EGUIET_ELEMENT, device->getGUIEnvironment(), device->getGUIEnvironment()->getRootGUIElement(), -1, rect<s32>(0,0,device->getVideoDriver()->getScreenSize().Width,device->getVideoDriver()->getScreenSize().Height)),
	device(device),
	cbk(cbk){
	IniFile guiIni;
	guiIni.setFromString(guiCodeString);
	irr::core::dimension2d<irr::u32> dim = Environment->getVideoDriver()->getScreenSize();
	win = addWindowForRatio(Environment, &guiIni, irr::core::rect<irr::s32>(dim.Width/4,dim.Height/20,3*dim.Width/4,dim.Height/2), isModal, this);
	win->getCloseButton()->setVisible(false);
	win->setDrawTitlebar(false);
	win->setDraggable(false);
	gui = new GUI(Environment, &guiIni, irr::core::rect<irr::s32>(0,0,win->getRelativePosition().getWidth(),win->getRelativePosition().getHeight()), win, true);
	ettitle = (IGUIStaticText*)gui->getElement("ttitle");
	ettitle->setText(title);
	eeedit = (IGUIEditBox*)gui->getElement("eedit");
	eeedit->setText(defaultText);
	if(isPasswordBox){
		eeedit->setPasswordBox(true, passwordChar);
	}
	ebok = (IGUIButton*)gui->getElement("bok");
	ebok->setText(positiveLabel);
	ebcancel = (IGUIButton*)gui->getElement("bcancel");
	ebcancel->setText(negativeLabel);
	Environment->setFocus(eeedit);
	//put cursor to the end
	SEvent event;
	event.EventType = EET_KEY_INPUT_EVENT;
	event.KeyInput = SEvent::SKeyInput{L'\0', KEY_END, 0, true, false, false};
	eeedit->OnEvent(event);
	event.KeyInput.PressedDown = false;
	eeedit->OnEvent(event);
}
	
EditBoxDialog::~EditBoxDialog(){
	delete gui;
}
	
bool EditBoxDialog::OnEvent(const irr::SEvent& event){
	bool res = irr::gui::IGUIElement::OnEvent(event);
	if(!res){//check because EditBoxDialog may have been deleted otherwise
		if(event.EventType==EET_GUI_EVENT){
			const SEvent::SGUIEvent& g = event.GUIEvent;
			if(g.EventType==EGET_BUTTON_CLICKED){
				if(g.Caller==ebok){
					cbk->OnResult(this, eeedit->getText(), true);
					remove();
					drop();
					return true;
				}else if(g.Caller==ebcancel){
					cbk->OnResult(this, eeedit->getText(), false);
					remove();
					drop();
					return true;
				}
			}else if(g.EventType==EGET_EDITBOX_ENTER){
				if(g.Caller==eeedit){
					bool res = irr::gui::IGUIElement::OnEvent(event);
					cbk->OnResult(this, eeedit->getText(), true);
					remove();
					drop();
					return res;
				}
			}
		}
	}
	return res;
}

bool startEditBoxDialog(std::wstring& out, irr::video::SColor bgColor, irr::IrrlichtDevice* device, const wchar_t* title, const wchar_t* positiveLabel, const wchar_t* negativeLabel, const wchar_t* defaultText, bool isModal, bool isPasswordBox, wchar_t passwordChar, const std::function<void()>& updateFunc){
	bool okPressed, finished = false;
	LambdaEditBoxCallback cbk([&okPressed, &out, &finished](EditBoxDialog* dialog, const wchar_t* text, bool positivePressed){
		finished = true;
		okPressed = positivePressed;
		if(okPressed){out = text;}
	});
	new EditBoxDialog(&cbk, device, title, positiveLabel, negativeLabel, defaultText, isModal, isPasswordBox, passwordChar);
	IVideoDriver* driver = device->getVideoDriver();
	IGUIEnvironment* env = device->getGUIEnvironment();
	while(device->run() && !finished){
		driver->beginScene(true, true, bgColor);
		env->drawAll();
		updateFunc();
		driver->endScene();
		device->yield();
	}
	return okPressed;
}
