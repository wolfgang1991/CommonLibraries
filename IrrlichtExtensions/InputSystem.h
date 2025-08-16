#ifndef INPUTSYSTEM_H_INCLUDED
#define INPUTSYSTEM_H_INCLUDED

class ICommonAppContext;

#include "ForwardDeclarations.h"
#include "TouchKeyboard.h"

#include <vector>
#include <map>

class IniFile;

//! DFAs to recognize invalid characters (e.g. for EditBoxes)
class IInvalidCharDetector{

	public:

	//read next char (true if valid, false if invalid)
	virtual bool nextChar(char c) = 0;

	//reset DFA to start state (required before each test for invalid chars)
	virtual void reset() = 0;

	virtual ~IInvalidCharDetector(){}

};

class InvalidIntDetector : public IInvalidCharDetector{

	public:

	InvalidIntDetector();

	bool nextChar(char c);

	void reset();

	private:
	bool firstChar;

};

class InvalidDoubleDetector : public IInvalidCharDetector{

	public:

	InvalidDoubleDetector();

	bool nextChar(char c);

	void reset();

	private:
	int z;//0: empty -,. possible; 1:-read . possible; 2:. read

};

//! Handles all input stuff
class InputSystem : public irr::IEventReceiver{

	private:
	ICommonAppContext* c;
	irr::IrrlichtDevice* device;
	irr::video::IVideoDriver* driver;
	irr::gui::IGUIEnvironment* env;

	irr::video::SColor inputColor;
	TouchKey* button;
	
	std::vector<IInput*> input;
	int activeInput;
	bool inputVisible;

	bool mouseDown;
	
	double time;
	int editState;//0: mouse released, 1: editbox just focussed, 2: wait until other events over

	std::map<irr::s32, IInvalidCharDetector*> dfa;

	//! returns true if the event shall be processed further
	bool processEvent(const irr::SEvent& event);
	
	void handleEditBoxMarking();

	public:
	
	//! keyboards: Keyboards definition (see structs in TouchKeyboard.h), changeTexturePath path for the icon for changing input methods
	InputSystem(ICommonAppContext* context, const std::vector<KeyboardDefinition>& keyboards, const std::string& changeTexturePath, const std::vector<IInput*>& additionalInputs = {});

	~InputSystem();

	bool OnEvent(const irr::SEvent& event);

	void render();

	void setColor(irr::video::SColor Color);

	//! useful in case the color is stored in a config file
	void loadInputSettingsFromIni(IniFile* ini, bool preferPlatformDependentInputDefaultSetting = false);
	
	void setChangeButtonLocation(const irr::core::rect<irr::s32>& rectangle);
	
	void setPlatformDependentInputPreferred(bool prefer);
	
	bool isPlatformDependentInputPreferred() const;

};

#endif
