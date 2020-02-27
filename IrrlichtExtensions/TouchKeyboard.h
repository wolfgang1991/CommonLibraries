#ifndef TOUCHKEYBOARD_H_INCLUDED
#define TOUCHKEYBOARD_H_INCLUDED

#include "IInput.h"
#include "TouchKey.h"

#include <list>
#include <vector>
#include <set>
#include <string>

class ICommonAppContext;

struct KeyDefinition{
	wchar_t firstChar;//when shift not pressed
	wchar_t secondChar;//when shift pressed
	irr::EKEY_CODE keyCode;
	irr::f64 size;//size of the key 1=normal key size, 2=double etc... (sum of sizes should equal the amount of keys per Line)
	std::string texturePath;//path of an override texture or empty string
};

struct KeyboardDefinition{
	irr::s32 keysPerLine;
	irr::f64 padding;//padding to the left/right, unit: #keys
	std::set<irr::s32> preferredInputIDs;//ids of the editoboxes for which this is the preferred input
	std::vector<std::vector<KeyDefinition>> lines;
};

class TouchKeyboard : public IInput{

	public:

	TouchKeyboard(ICommonAppContext* context, irr::video::SColor Color, const KeyboardDefinition& keyboardDef);

	void render();

	bool processEvent(const irr::SEvent& event);

	void setColor(irr::video::SColor Color);

	void OnSelect();

	void OnDeSelect();
	
	bool isPreferredInput(irr::s32 editboxID);

	private:

	void emitEvent(std::list<TouchKey>::iterator it);

	ICommonAppContext* c;
	irr::video::IVideoDriver* driver;
	Drawer2D* drawer;
	irr::IrrlichtDevice* device;
	irr::video::SColor color;
	int w, h;
	std::vector< std::list<TouchKey> > key;
	
	int keyboardY;

	irr::SEvent backspaceEvent;

	bool shiftPressed;
	
	irr::core::dimension2d<irr::u32> keySize;
	
	irr::f64 paddingX;
	std::set<irr::s32> preferredInputIDs;

};

#endif
