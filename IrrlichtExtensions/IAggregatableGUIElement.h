#ifndef IAggregatableGUIElement_H_INCLUDED
#define IAggregatableGUIElement_H_INCLUDED

#include "IExtendableSkin.h"
#include "ForwardDeclarations.h"

#include <IGUIElement.h>

#include <functional>

//! useful to get the appopriate extension for rendering derived aggregatable gui elements 
extern const std::string aggregatableSkinExtensionName;

//! Skin extensions for elements derived from IAggregatableGUIElement must inherit from this class 
class IAggregatableSkinExtension : public ISkinExtension{

	protected:
	
	IExtendableSkin* skin;
	bool highlightIfActive;
	bool highlightIfPressed;
	
	public:
	
	IAggregatableSkinExtension(IExtendableSkin* skin, bool highlightIfActive, bool highlightIfPressed = false);
	
	virtual ~IAggregatableSkinExtension(){}
	
	const std::string& getName() final;
	
	virtual void drawHighlight(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& highlightRect, const irr::core::rect<irr::s32>* clip);
	
	virtual void drawPressedHighlight(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& highlightRect, const irr::core::rect<irr::s32>* clip);
	
};

//! Useful for aggregatable gui elements which do not need custom extensions for the skin
typedef IAggregatableSkinExtension DefaultAggregatableSkin;

//! Interface and default implementations for aggregatable GUI Elements.
//! The purpose is to arrange GUI Elements in a specific pattern e.g. horizontally or vertically
class IAggregatableGUIElement : public irr::gui::IGUIElement{

	protected:
	
	irr::f32 recommendedSpace;
	irr::f32 aspectRatio;
	irr::f32 recommendedActiveSpace;
	irr::f32 activeAspectRatio;
	bool maintainAspectRatio;
	bool isActivateAble;
	bool active;
	bool activationLock;
	bool pressedInside;
	
	void* data;
	
	std::function<void(IAggregatableGUIElement* ele)> onClick;
	std::function<void(IAggregatableGUIElement* ele)> onLongTap;
	double longTapTime;
	
	irr::core::rect<irr::s32> rectWhenPressed;
	double timeWhenPressed;
	
	public:
	
	//! rectangle may be later overriden by parents
	//! data is meant to store useful stuff for identification etc depending on the use case
	//! automatically adds it to the parent or root gui element and drops the "local" reference
	IAggregatableGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio, irr::f32 recommendedActiveSpace, irr::f32 activeAspectRatio, bool maintainAspectRatio, bool isActivateAble, irr::s32 id, void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	virtual ~IAggregatableGUIElement(){}
	
	virtual irr::f32 getAspectRatio();
	
	virtual bool mustMaintainAspectRatio();
	
	//! elements recommended space in parts of the parent's space ( width (horizontal aggregation) / height (vertical aggregation) )
	//! MUST return 0.f if Element is invisible
	virtual irr::f32 getRecommendedSpace();
	
	//! an updateAbsolutePosition on it's parent is recommended after setting the spaces
	virtual void setRecommendedSpace(irr::f32 space);
	
	//! an updateAbsolutePosition on it's parent is recommended after setting the spaces
	virtual void setRecommendedActiveSpace(irr::f32 space);
	
	virtual irr::f32 getThisSpace(irr::f32 parentSpace);
	
	//! calculates the other (horizontal => vertical, vertical => horizontal) space requirement for a given parent aggregation space, fallback in case of disregarded aspect ratio is used
	virtual irr::f32 getOtherSpaceForAspectRatio(bool parentHorizontal, irr::f32 parentSpace, irr::f32 fallback = 0.f);
	
	//! true if active/selected
	virtual bool isActive();
	
	virtual void draw();
	
	virtual bool OnEvent(const irr::SEvent& event);
	
	//! lock activation possibility (useful if events shall be processed by a parent without selecting the children)
	virtual void setActivationLock(bool on);
	
	virtual void setActive(bool active, bool emitEventOnChange = true);
	
	virtual void* getData();
	
	virtual void setData(void* data);
	
	virtual void setActivableFlag(bool isActivateAble);
	
	virtual bool getActivableFlag() const;
	
	virtual void setOnClickCallback(const std::function<void(IAggregatableGUIElement* ele)>& callback);
	
	//! longTapTime in s
	virtual void setLongTapCallback(const std::function<void(IAggregatableGUIElement* ele)>& callback, double longTapTime = 1.0);
	
};

//! E.g. useful to define empty spaces in aggregations
class EmptyGUIElement : public IAggregatableGUIElement{
	
	public:
	
	EmptyGUIElement(irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::f32 aspectRatio = 1.f, bool maintainAspectRatio = false, bool isActivateAble = false, irr::s32 id = -1, void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0)):
			IAggregatableGUIElement(environment, recommendedSpace, aspectRatio, recommendedSpace, aspectRatio, maintainAspectRatio, isActivateAble, id, data, parent, rectangle){}
};

#endif
