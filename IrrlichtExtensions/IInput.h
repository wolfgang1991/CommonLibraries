#ifndef IINPUT_H_INCLUDED
#define IINPUT_H_INCLUDED

#include <SColor.h>
#include <IEventReceiver.h>

#define INT_EDIT -1
#define DOUBLE_EDIT -2
#define STRING_EDIT -3
#define NO_EDIT -4
#define MULTILINE_STRING_EDIT -5
#define DESTINATION_EDIT -6
#define TERMINAL_EDIT -7

class IInput{

	public:

	//! render and update the input method
	virtual void render() = 0;

	//! returns true if the event shall be processed further
	virtual bool processEvent(const irr::SEvent& event) = 0;

	//! gets called when the input method is selected
	virtual void OnSelect() = 0;

	//! sets the color of the input method
	virtual void setColor(irr::video::SColor Color) = 0;

	//! gets called when the input method is deselected
	virtual void OnDeSelect() = 0;
	
	//! returns true if it is the preferred input method for some kind of editbox
	virtual bool isPreferredInput(irr::s32 editboxID){return false;}

	virtual ~IInput(){}

};

#endif
