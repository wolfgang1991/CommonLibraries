#ifndef CMBOX_H_INCLUDED
#define CMBOX_H_INCLUDED

#include <ForwardDeclarations.h>

#include <IGUIElement.h>

#include <string>

//! more beatiful messagebox which can not be dragged off the screen
//! deletes itself as soon as the user exits the box
class CMBox : public irr::gui::IGUIElement{

	protected:
	
	void OnNegative();
	
	void OnPositive();

	public:

	irr::gui::IGUIEnvironment* env;
	irr::video::IVideoDriver* driver;
	irr::gui::IGUIWindow* win;
	irr::gui::IGUIStaticText* stext;
	irr::gui::IGUIButton* pos;
	irr::gui::IGUIButton* neg;

	int w, h;

	CMBox(irr::IrrlichtDevice* device, std::wstring text, irr::f32 maxW = 0.75, irr::f32 maxH = 0.75, const wchar_t* positive = L"Ok", const wchar_t* negative = NULL, irr::s32 posId = -1, irr::s32 negId = -1, bool modal = true, irr::gui::EGUI_ALIGNMENT horizontal = irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUI_ALIGNMENT vertical = irr::gui::EGUIA_UPPERLEFT);

	virtual ~CMBox();

	bool OnEvent(const irr::SEvent& event);

	void OnPostRender(irr::u32 timeMs);

};

#endif
