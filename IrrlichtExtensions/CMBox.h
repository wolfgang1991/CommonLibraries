#ifndef CMBOX_H_INCLUDED
#define CMBOX_H_INCLUDED

#include <ForwardDeclarations.h>

#include <IGUIElement.h>

#include <string>
#include <functional>

//! more beatiful messagebox which can not be dragged off the screen
//! deletes itself as soon as the user exits the box
class CMBox : public irr::gui::IGUIElement{

	protected:
	
	void OnNegative();
	
	void OnPositive();
	
	irr::core::dimension2d<irr::u32> screenSize;
	bool closeOnScreenResize;
	bool mustDelete;
	
	std::function<void()> onPositive, onNegative;

	public:

	irr::gui::IGUIEnvironment* env;
	irr::video::IVideoDriver* driver;
	irr::gui::IGUIWindow* win;
	irr::gui::IGUIStaticText* stext;
	irr::gui::IGUIButton* pos;
	irr::gui::IGUIButton* neg;

	int w, h;

	CMBox(irr::IrrlichtDevice* device, std::wstring text, irr::f32 maxW = 0.75, irr::f32 maxH = 0.75, const wchar_t* positive = L"OK", const wchar_t* negative = NULL, irr::s32 posId = -1, irr::s32 negId = -1, bool modal = true, irr::gui::EGUI_ALIGNMENT horizontal = irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUI_ALIGNMENT vertical = irr::gui::EGUIA_UPPERLEFT);

	virtual ~CMBox();

	bool OnEvent(const irr::SEvent& event);

	void OnPostRender(irr::u32 timeMs);
	
	void setCloseOnScreenResize(bool closeOnScreenResize);
	
	//! alternative way to handle results (normal is via events)
	void setPositiveCallback(const std::function<void()>& onPositive);
	
	//! alternative way to handle results (normal is via events)
	void setNegativeCallback(const std::function<void()>& onNegative);

};

void bringToFrontRecursive(irr::gui::IGUIElement* ele);

#endif
