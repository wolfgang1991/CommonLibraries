#ifndef AggregatableGUIElementAdapter_H_INCLUDED
#define AggregatableGUIElementAdapter_H_INCLUDED

#include "IAggregatableGUIElement.h"

class AggregatableGUIElementAdapter : public IAggregatableGUIElement{

	public:

	AggregatableGUIElementAdapter(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, irr::gui::IGUIElement* child, bool isActivateAble, irr::s32 id, void* data = NULL, irr::gui::IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	void updateAbsolutePosition();

};

AggregatableGUIElementAdapter* addAggregatableStaticText(irr::gui::IGUIEnvironment* env, const wchar_t* text, irr::gui::EGUI_ALIGNMENT hori, irr::gui::EGUI_ALIGNMENT verti, irr::f32 weight);

AggregatableGUIElementAdapter* addAggregatableButton(irr::gui::IGUIEnvironment* env, const wchar_t* text, irr::f32 weight, irr::s32 id = -1);

AggregatableGUIElementAdapter* addAggregatableEditBox(irr::gui::IGUIEnvironment* env, const wchar_t* text, irr::f32 weight, bool border = true, irr::s32 id = -1);

AggregatableGUIElementAdapter* addAggregatableCheckBox(irr::gui::IGUIEnvironment* env, const wchar_t* text, bool checked, irr::f32 weight, irr::s32 id = -1);

AggregatableGUIElementAdapter* addAggregatableComboBox(irr::gui::IGUIEnvironment* env, irr::f32 weight, irr::s32 id = -1);

AggregatableGUIElementAdapter* addAggregatableListBox(irr::gui::IGUIEnvironment* env, irr::f32 weight, irr::s32 id = -1);

//! useful in case an adapter is sometimes used to avoid unnecessary ifs
irr::gui::IGUIElement* getFirstGUIElementChild(irr::gui::IGUIElement* ele);

#endif
