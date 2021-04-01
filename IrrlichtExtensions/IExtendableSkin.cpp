#include <IExtendableSkin.h>
#include <Drawer2D.h>

#include <set>

static std::set<void*> extendableSkins;

bool isExtendableSkin(void* something){
	return extendableSkins.find(something)!=extendableSkins.end();
}

IExtendableSkin::IExtendableSkin(irr::gui::IGUISkin* parent, Drawer2D* drawer):
	drawer(drawer),
	device(drawer->getDevice()),
	parent(parent){
	extendableSkins.insert(this);
}

IExtendableSkin::~IExtendableSkin(){
	extendableSkins.erase(extendableSkins.find(this));
	for(auto it = extensions.begin(); it != extensions.end(); ++it){
		for(auto it2 = it->second.begin(); it2 != it->second.end(); ++it2){
			delete it2->second;
		}
	}
}

void IExtendableSkin::registerExtension(ISkinExtension* extension, uint32_t styleId){
	if(extension==NULL){return;}
	auto it = extensions.find(extension->getName());
	bool added = false;
	if(it!=extensions.end()){
		auto it2 = it->second.find(styleId);
		if(it2!=it->second.end()){
			added = true;
			delete it2->second;
			it2->second = extension;
		}
	}
	if(!added){
		extensions[extension->getName()][styleId] = extension;
	}
}
	
ISkinExtension* IExtendableSkin::getExtension(const std::string& extensionName, uint32_t styleId){
	auto it = extensions.find(extensionName);
	if(it==extensions.end()){return NULL;}
	auto it2 = it->second.find(styleId);
	if(it2==it->second.end()){return NULL;}
	return it2->second;
}

irr::gui::IGUISkin* IExtendableSkin::getParentSkin(){
	return parent;
}

irr::IrrlichtDevice* IExtendableSkin::getDevice(){
	return device;
}

Drawer2D* IExtendableSkin::getDrawer2D(){
	return drawer;
}

irr::video::SColor IExtendableSkin::getColor(irr::gui::EGUI_DEFAULT_COLOR color) const{
	return parent->getColor(color);
}

void IExtendableSkin::setColor(irr::gui::EGUI_DEFAULT_COLOR which, irr::video::SColor newColor){
	parent->setColor(which, newColor);
}

irr::s32 IExtendableSkin::getSize(irr::gui::EGUI_DEFAULT_SIZE size) const{
	return parent->getSize(size);
}

const wchar_t* IExtendableSkin::getDefaultText(irr::gui::EGUI_DEFAULT_TEXT text) const{
	return parent->getDefaultText(text);
}

void IExtendableSkin::setDefaultText(irr::gui::EGUI_DEFAULT_TEXT which, const wchar_t* newText){
	parent->setDefaultText(which, newText);
}

void IExtendableSkin::setSize(irr::gui::EGUI_DEFAULT_SIZE which, irr::s32 size){
	parent->setSize(which, size);
}

irr::gui::IGUIFont* IExtendableSkin::getFont(irr::gui::EGUI_DEFAULT_FONT which) const{
	return parent->getFont(which);
}

void IExtendableSkin::setFont(irr::gui::IGUIFont* font, irr::gui::EGUI_DEFAULT_FONT which){
	parent->setFont(font, which);
}

irr::gui::IGUISpriteBank* IExtendableSkin::getSpriteBank() const{
	return parent->getSpriteBank();
}

void IExtendableSkin::setSpriteBank(irr::gui::IGUISpriteBank* bank){
	parent->setSpriteBank(bank);
}

irr::u32 IExtendableSkin::getIcon(irr::gui::EGUI_DEFAULT_ICON icon) const{
	return parent->getIcon(icon);
}

void IExtendableSkin::setIcon(irr::gui::EGUI_DEFAULT_ICON icon, irr::u32 index){
	parent->setIcon(icon, index);
}

void IExtendableSkin::draw3DButtonPaneStandard(irr::gui::IGUIElement* element,
		const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip){
	parent->draw3DButtonPaneStandard(element, rect, clip);
}

void IExtendableSkin::draw3DButtonPanePressed(irr::gui::IGUIElement* element,
		const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip){
	return parent->draw3DButtonPanePressed(element, rect, clip);
}

void IExtendableSkin::draw3DSunkenPane(irr::gui::IGUIElement* element,
		irr::video::SColor bgcolor, bool flat, bool fillBackGround,
		const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip){
	parent->draw3DSunkenPane(element, bgcolor, flat, fillBackGround, rect, clip);
}

irr::core::rect<irr::s32> IExtendableSkin::draw3DWindowBackground(irr::gui::IGUIElement* element,
		bool drawTitleBar, irr::video::SColor titleBarColor,
		const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip,
		irr::core::rect<irr::s32>* checkClientArea){
	return parent->draw3DWindowBackground(element, drawTitleBar, titleBarColor, rect, clip, checkClientArea);
}

void IExtendableSkin::draw3DMenuPane(irr::gui::IGUIElement* element,
		const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip){
	parent->draw3DMenuPane(element, rect, clip);
}

void IExtendableSkin::draw3DToolBar(irr::gui::IGUIElement* element,
		const irr::core::rect<irr::s32>& rect,
		const irr::core::rect<irr::s32>* clip){
	parent->draw3DToolBar(element, rect, clip);
}

void IExtendableSkin::draw3DTabButton(irr::gui::IGUIElement* element, bool active,
		const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip, irr::gui::EGUI_ALIGNMENT alignment){
	return parent->draw3DTabButton(element, active, rect, clip, alignment);
}

void IExtendableSkin::draw3DTabBody(irr::gui::IGUIElement* element, bool border, bool background,
		const irr::core::rect<irr::s32>& rect, const irr::core::rect<irr::s32>* clip, irr::s32 tabHeight, irr::gui::EGUI_ALIGNMENT alignment){
	return parent->draw3DTabBody(element, border, background, rect, clip, tabHeight, alignment);
}

void IExtendableSkin::drawIcon(irr::gui::IGUIElement* element, irr::gui::EGUI_DEFAULT_ICON icon,
		const irr::core::position2di position, irr::u32 starttime, irr::u32 currenttime,
		bool loop, const irr::core::rect<irr::s32>* clip){
	return parent->drawIcon(element, icon, position, starttime, currenttime, loop, clip);
}

void IExtendableSkin::draw2DRectangle(irr::gui::IGUIElement* element, const irr::video::SColor &color,
		const irr::core::rect<irr::s32>& pos, const irr::core::rect<irr::s32>* clip){
	return parent->draw2DRectangle(element, color, pos, clip);
}

irr::gui::EGUI_SKIN_TYPE IExtendableSkin::getType() const{
	return irr::gui::EGST_UNKNOWN;
}

