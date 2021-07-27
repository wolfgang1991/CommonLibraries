#ifndef KEYINPUT_H_INCLUDED
#define KEYINPUT_H_INCLUDED

#include "IInput.h"
#include "TouchKey.h"
#include "ForwardDeclarations.h"

class ICommonAppContext;

//! OS dependent input stuff
class KeyInput : public IInput{

	private:

	ICommonAppContext* c;
	bool preferred;

	public:
	
	//! isPreferred: true if this OS dependent input is preferred over platform independent implementations
	KeyInput(ICommonAppContext* context, bool preferred = false);

	void render();

	bool processEvent(const irr::SEvent& event);

	void OnSelect();

	void OnDeSelect();

	void setColor(irr::video::SColor Color){};
	
	bool isPreferredInput(irr::s32 editboxID);
	
	void setPreferred(bool prefer);
	
	bool isPreferred() const;

};

#endif
