#ifndef NOTIFICATIONBOX_H_INCLUDED
#define NOTIFICATIONBOX_H_INCLUDED

#include <CMBox.h>

class NotificationBox : public CMBox{

	private:
	
	irr::u32 timeoutMs, startMs;
	std::wstring label;
	
	public:
	
	//! after the timeout the positive button (if available) is "pressed" automatically
	NotificationBox(irr::u32 timeoutMs, irr::IrrlichtDevice* device, std::wstring text, irr::f32 maxW = 0.75, irr::f32 maxH = 0.75, const wchar_t* positive = L"Ok", const wchar_t* negative = NULL, irr::s32 posId = -1, irr::s32 negId = -1, bool modal = false, irr::gui::EGUI_ALIGNMENT horizontal = irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUI_ALIGNMENT vertical = irr::gui::EGUIA_UPPERLEFT);

	void OnPostRender(irr::u32 timeMs);
	
};

#endif
