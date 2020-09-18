#include <AggregatableGUIElementAdapter.h>

#include <IGUIStaticText.h>
#include <IGUIButton.h>
#include <IGUIEditBox.h>
#include <IGUICheckBox.h>
#include <IGUIComboBox.h>

using namespace irr;
using namespace gui;
using namespace core;

AggregatableGUIElementAdapter::AggregatableGUIElementAdapter(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, irr::gui::IGUIElement* child, bool isActivateAble, irr::s32 id, void* data, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, aspectRatio, recommendedSpace, aspectRatio, maintainAspectRatio, isActivateAble, id, data, parent, rectangle){
	addChild(child);
}

void AggregatableGUIElementAdapter::updateAbsolutePosition(){
	recalculateAbsolutePosition(false);
	for(irr::core::list<IGUIElement*>::Iterator it = Children.begin(); it != Children.end(); ++it){
		(*it)->setRelativePosition(irr::core::rect<irr::s32>(0,0,AbsoluteRect.getWidth(),AbsoluteRect.getHeight()));
		(*it)->updateAbsolutePosition();
	}
}

AggregatableGUIElementAdapter* addAggregatableStaticText(IGUIEnvironment* env, const wchar_t* text, EGUI_ALIGNMENT hori, EGUI_ALIGNMENT verti, irr::f32 weight){
	IGUIStaticText* t = env->addStaticText(text, rect<s32>(0,0,0,0), false);
	t->setTextAlignment(hori, verti);
	return new AggregatableGUIElementAdapter(env, weight, 1.f, false, t, false, 0);
}

AggregatableGUIElementAdapter* addAggregatableButton(IGUIEnvironment* env, const wchar_t* text, irr::f32 weight, irr::s32 id){
	return new AggregatableGUIElementAdapter(env, weight, 1.f, false, env->addButton(rect<s32>(0,0,0,0), NULL, id, text, NULL), false, 0);
}

AggregatableGUIElementAdapter* addAggregatableEditBox(irr::gui::IGUIEnvironment* env, const wchar_t* text, irr::f32 weight, bool border, irr::s32 id){
	return new AggregatableGUIElementAdapter(env, weight, 1.f, false, env->addEditBox(text, rect<s32>(0,0,0,0), border, NULL, id), false, 0);
}

AggregatableGUIElementAdapter* addAggregatableCheckBox(irr::gui::IGUIEnvironment* env, const wchar_t* text, bool checked, irr::f32 weight, irr::s32 id){
	return new AggregatableGUIElementAdapter(env, weight, 1.f, false, env->addCheckBox(checked, rect<s32>(0,0,0,0), NULL, id, text), false, 0);
}

AggregatableGUIElementAdapter* addAggregatableComboBox(irr::gui::IGUIEnvironment* env, irr::f32 weight, irr::s32 id){
	return new AggregatableGUIElementAdapter(env, weight, 1.f, false, env->addComboBox(rect<s32>(0,0,0,0), NULL, id), false, 0);
}

irr::gui::IGUIElement* getFirstGUIElementChild(irr::gui::IGUIElement* ele){
	if(ele){
		return ele->getChildren().empty()?NULL:*(ele->getChildren().begin());
	}
	return NULL;
}
