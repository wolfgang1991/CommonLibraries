#include <ICommonAppContext.h>
#include <TouchKeyboard.h>
#include <font.h>
#include <FlexibleFont.h>
#include <mathUtils.h>

#include <IrrlichtDevice.h>
#include <IGUIElement.h>
#include <IGUIEnvironment.h>
#include <IGUIEditBox.h>

using namespace std;
using namespace irr;
using namespace core;
using namespace video;
using namespace gui;

TouchKeyboard::TouchKeyboard(ICommonAppContext* context, SColor Color, const KeyboardDefinition& keyboardDef):key(keyboardDef.lines.size()){
	c = context;
	device = c->getIrrlichtDevice();
	driver = device->getVideoDriver();
	drawer = c->getDrawer();
	color = Color;
	w = device->getVideoDriver()->getScreenSize().Width;
	h = device->getVideoDriver()->getScreenSize().Height;
	const std::vector<std::vector<KeyDefinition>>& lines = keyboardDef.lines;
	f64 keyHeight = Min((h/2.0)/lines.size(), c->getRecommendedButtonHeight()*1.5);
	keyboardY = rd<f64,int>(h-keyHeight*lines.size());
	vector2d<f64> off((f64)w/((f64)keyboardDef.keysPerLine+2.0*keyboardDef.padding), keyHeight);//(h/2.0)/5.0);
	paddingX = keyboardDef.padding*off.X;
	vector2d<f64> p(paddingX, keyboardY);
	keySize = dimension2d<irr::u32>(3*off.X/4, 3*off.Y/4);
	for(uint32_t i=0; i<lines.size(); i++){
		const std::vector<KeyDefinition>& line = lines[i];
		p.X = paddingX;
		for(uint32_t j=0; j<line.size(); j++){
			const KeyDefinition& k = line[j];
			ITexture* overrideTex = k.texturePath.empty()?NULL:driver->getTexture(c->getPath(k.texturePath).c_str());
			key[i].push_back(TouchKey(c, k.firstChar, k.secondChar, k.keyCode, rect<irr::s32>(rdVector2D<f64,s32>(p), rdVector2D<f64,s32>(p+vector2d<f64>(k.size*off.X, off.Y))), overrideTex));
			p.X += k.size*off.X;
		}
		p.Y += off.Y;
	}
	shiftPressed = false;
	backspaceEvent.EventType = EET_KEY_INPUT_EVENT;
	backspaceEvent.KeyInput.Char = 0;
	backspaceEvent.KeyInput.Control = false;
	backspaceEvent.KeyInput.Key = KEY_BACK;
	backspaceEvent.KeyInput.Shift = false;
	preferredInputIDs = keyboardDef.preferredInputIDs;
}

void TouchKeyboard::render(){
	IGUIEnvironment* env = device->getGUIEnvironment();
	FlexibleFont* best = c->getFlexibleFont();
	irr::f32 optimalScale = best->calculateOptimalScale(L"O", keySize);
	vector2d<f32> currentScale = best->getDefaultScale();
	best->setDefaultScale(vector2d<f32>(optimalScale,optimalScale));
	for(uint32_t i=0; i<key.size(); i++){
		for(std::list<TouchKey>::iterator it = key[i].begin(); it != key[i].end(); ++it){
			it->setShiftPressed(shiftPressed);
			if(it->render(best, true, color)){
				emitEvent(it);
			}
		}
	}
	SColor lineColor(color);
	lineColor.setAlpha(lineColor.getAlpha()<200?lineColor.getAlpha():200);
	IGUIElement* ele = env->getFocus();
	if(ele){
		if(ele->getType()==EGUIET_EDIT_BOX){
			IGUIEditBox* edit = (IGUIEditBox*)ele;
			if(edit->getID()!=MULTILINE_STRING_EDIT && !edit->isPasswordBox()){
				int fh = best->getDimension(L"O").Height;
				rect<s32> r(0, keyboardY-fh-4, w, keyboardY);
				driver->draw2DRectangle(lineColor, r);
				best->draw(ele->getText(), r, SColor(255, 0, 0, 0), true, true, &r);
			}
		}
	}
	if(paddingX>=1.0){
		driver->draw2DRectangle(lineColor, rect<s32>(0,keyboardY,rd<f64,s32>(paddingX),h));
		driver->draw2DRectangle(lineColor, rect<s32>(rd<f64,s32>(w-paddingX),keyboardY,w,h));
	}
	best->setDefaultScale(currentScale);
}

void TouchKeyboard::setColor(irr::video::SColor Color){
	color = Color;
}

void TouchKeyboard::emitEvent(std::list<TouchKey>::iterator it){
	SEvent toPost;
	toPost.KeyInput = it->getEvent();
	toPost.EventType = EET_KEY_INPUT_EVENT;
	if(toPost.KeyInput.PressedDown && it->mousePressedDownOutside() && toPost.KeyInput.Key != KEY_BACK && toPost.KeyInput.Key != KEY_RETURN && toPost.KeyInput.Key != KEY_LEFT && toPost.KeyInput.Key != KEY_RIGHT && toPost.KeyInput.Key != KEY_SHIFT){
		backspaceEvent.KeyInput.PressedDown = true;
		device->postEventFromUser(backspaceEvent);//execute backspace if moving over multiple keys to remove the old ones
		backspaceEvent.KeyInput.PressedDown = false;
		device->postEventFromUser(backspaceEvent);
	}
	device->postEventFromUser(toPost);
	if(toPost.KeyInput.Key == KEY_SHIFT){
		if(!toPost.KeyInput.PressedDown){
			shiftPressed = !shiftPressed;
		}
	}else{
		shiftPressed = false;
	}
}

bool TouchKeyboard::processEvent(const irr::SEvent& event){
	bool ret = true;
	if(event.EventType == EET_MOUSE_INPUT_EVENT){
		if(event.MouseInput.Y>=keyboardY){ret = false;}
		for(uint32_t i=0; i<key.size(); i++){
			for(std::list<TouchKey>::iterator it = key[i].begin(); it != key[i].end(); ++it){
				if(it->processMouseEvent(event.MouseInput)){//TODO: multitouch events
					emitEvent(it);
				}
			}
		}
	}
	return ret;
}

void TouchKeyboard::OnSelect(){
	for(uint32_t i=0; i<key.size(); i++){
		for(std::list<TouchKey>::iterator it = key[i].begin(); it != key[i].end(); ++it){
			it->reset();
		}
	}
}

void TouchKeyboard::OnDeSelect(){
	OnSelect();
}

bool TouchKeyboard::isPreferredInput(irr::s32 editboxID){
	return preferredInputIDs.find(editboxID)!=preferredInputIDs.end();
}
