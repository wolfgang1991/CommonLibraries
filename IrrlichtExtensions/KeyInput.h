#ifndef KEYINPUT_H_INCLUDED
#define KEYINPUT_H_INCLUDED

#include "IInput.h"
#include "TouchKey.h"
#include "ForwardDeclarations.h"

class ICommonAppContext;

//! OS dependent input stuff
class KeyInput : public IInput{

	public:

	KeyInput(ICommonAppContext* context);

	void render();

	bool processEvent(const irr::SEvent& event);

	void OnSelect();

	void OnDeSelect();

	void setColor(irr::video::SColor Color){};

	private:

	ICommonAppContext* c;

};

#endif
