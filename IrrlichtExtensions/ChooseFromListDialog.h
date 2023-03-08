#ifndef ChooseFromListDialog_H_INCLUDED
#define ChooseFromListDialog_H_INCLUDED

#include "ForwardDeclarations.h"

#include <SColor.h>
#include <IGUIWindow.h>

#include <vector>
#include <string>

//! creates a list of buttons inside a window via aggregate gui elements (independent of EventReceiver implementation)
irr::gui::IGUIWindow* createChooseFromListDialog(irr::IrrlichtDevice* device, std::vector<irr::gui::IGUIButton*>& buttonsOut, const std::vector<int32_t>& buttonIds, const std::vector<std::wstring>& buttonLabels, const std::wstring& headline, int32_t aggregationId, int32_t aggregatableId, bool modal = true, bool cancelByClickOutside = false);

//! returns the selected index (dependent on EventReceiver implementation)
uint32_t startChooseFromListDialog(irr::IrrlichtDevice* device, const std::vector<int32_t>& buttonIds, const std::vector<std::wstring>& buttonLabels, const std::wstring& headline, int32_t aggregationId, int32_t aggregatableId, irr::video::SColor bgColor, bool modal = true);

//A window "adapter" which closes the windows with EGET_MESSAGEBOX_NO event in case of click outside window
class OutsideCancelWindow : public irr::gui::IGUIWindow{

	private:
	
	irr::gui::IGUIWindow* win;

	public:

	OutsideCancelWindow(irr::gui::IGUIEnvironment* environment, irr::gui::IGUIWindow* win, irr::s32 id = -1);

	irr::gui::IGUIButton* getCloseButton() const;

	irr::gui::IGUIButton* getMinimizeButton() const;

	irr::gui::IGUIButton* getMaximizeButton() const;

	bool isDraggable() const;

	void setDraggable(bool draggable);

	void setDrawBackground(bool draw);

	bool getDrawBackground() const;

	void setDrawTitlebar(bool draw);

	bool getDrawTitlebar() const;

	irr::core::rect<irr::s32> getClientRect() const;
	
	bool OnEvent(const irr::SEvent& event);
	
 };

#endif
