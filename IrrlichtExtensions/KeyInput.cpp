#include <ICommonAppContext.h>
#include <KeyInput.h>

#include <IrrlichtDevice.h>

using namespace irr;

KeyInput::KeyInput(ICommonAppContext* context){
	c = context;
}

void KeyInput::render(){
	//nothing to do
}

bool KeyInput::processEvent(const irr::SEvent& event){
	return true;
}

void KeyInput::OnSelect(){
	c->setSoftInputVisibility(true);
}

void KeyInput::OnDeSelect(){
	c->setSoftInputVisibility(false);
}

