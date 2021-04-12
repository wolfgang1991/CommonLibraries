#include <ICommonAppContext.h>
#include <KeyInput.h>

#include <IrrlichtDevice.h>

using namespace irr;

KeyInput::KeyInput(ICommonAppContext* context, bool preferred):preferred(preferred){
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

bool KeyInput::isPreferredInput(irr::s32 editboxID){
	return preferred;
}

void KeyInput::setPreferred(bool prefer){
	preferred = prefer;
}

bool KeyInput::isPreferred() const{
	return preferred;
}
