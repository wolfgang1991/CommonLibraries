#ifndef CallbackInsertGUIElement_H_INCLUDED
#define CallbackInsertGUIElement_H_INCLUDED

#include "IAggregatableGUIElement.h"

#include <functional>

//! Insert custom rendering callbacks into the GUI tree without explictly creating a new GUI Element (quick & dirty way)
class CallbackInsertGUIElement : public IAggregatableGUIElement{

	protected:
	
	std::function<bool(const irr::SEvent&)> onEventCbk;
	std::function<void(irr::u32)> onPostRenderCbk;
	std::function<void()> drawCbk;
	std::function<void()> onDeleteCbk;
	
	public:
	
	//! ctor useful if used in aggregations
	CallbackInsertGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, irr::f32 recommendedActiveSpace, irr::f32 activeAspectRatio, bool maintainAspectRatio, bool isActivateAble, const std::function<void()>& drawCbk = [](){}, const std::function<bool(const irr::SEvent&)>& onEventCbk = [](const irr::SEvent& event){return false;}, const std::function<void(irr::u32)>& onPostRenderCbk = [](irr::u32 timeMS){}, const std::function<void()>& onDeleteCbk = [](){}, irr::s32 id = -1, void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	//! ctor useful if used as overlay of a parent gui element (uses the same width/height)
	CallbackInsertGUIElement(irr::gui::IGUIEnvironment* environment, IGUIElement* parent, const std::function<void()>& drawCbk = [](){}, const std::function<bool(const irr::SEvent&)>& onEventCbk = [](const irr::SEvent& event){return false;}, const std::function<void(irr::u32)>& onPostRenderCbk = [](irr::u32 timeMS){}, const std::function<void()>& onDeleteCbk = [](){});
	
	virtual ~CallbackInsertGUIElement();
	
	void setDeleteCallback(const std::function<void()>& onDeleteCbk);
	
	void setDrawCallback(const std::function<void()>& drawCbk);
	
	void setOnPostRenderCallback(const std::function<void(irr::u32)>& onPostRenderCbk);
	
	void setOnEventCallback(const std::function<bool(const irr::SEvent&)>& onEventCbk);
	
	bool OnEvent(const irr::SEvent& event);
	
	void OnPostRender(irr::u32 timeMs);
	
	void draw();
	
	//! makes it small that it doesn't swallow event and still renders on the whole screen if necessary
	void collapse();
	
};

#endif
