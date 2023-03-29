#ifndef BeautifulCheckBox_H_INCLUDED
#define BeautifulCheckBox_H_INCLUDED

#include "BeautifulGUIText.h"

//! A BeautifulCheckBox is a non activable aggregateable gui element which emits checkbox events, it also uses the regular skin functions for checkbox drawing. Therefore the usual checkbox styles apply.
//! The aggregation does not have any border by itself and the id can be used to define a button style.
class BeautifulCheckBox : public BeautifulGUIText{

	protected:
	
	bool checked;
	bool pressed;
	
	irr::SEvent event;
	irr::SEvent checkBoxEvent;
	irr::core::rect<irr::s32> pressedStart;
	
	public:
	
	//! id is NOT used for fetching a ISkinExtension here (so arbitrary values are possible)
	BeautifulCheckBox(const wchar_t* text, irr::video::SColor color, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id = -1, irr::f32 scale = 1.f, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	//! like other constructor but with standard textcolor EGDC_BUTTON_TEXT (needs to be updated if changed by setAllBeautifulTextStandardColor)
	BeautifulCheckBox(const wchar_t* text, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id = -1, irr::f32 scale = 1.f, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	void draw();
	
	bool OnEvent(const irr::SEvent& event);
	
	bool isChecked() const;
	
	void setChecked(bool checked);
	
};

#endif
