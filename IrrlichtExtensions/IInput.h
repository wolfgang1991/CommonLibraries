#ifndef IINPUT_H_INCLUDED
#define IINPUT_H_INCLUDED

#include <SColor.h>
#include <IEventReceiver.h>

#include "EditConstants.h"

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
