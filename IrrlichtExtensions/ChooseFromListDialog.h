#ifndef ChooseFromListDialog_H_INCLUDED
#define ChooseFromListDialog_H_INCLUDED

#include <ForwardDeclarations.h>

#include <SColor.h>

#include <vector>
#include <string>

//! creates a list of buttons inside a window via aggregate gui elements (independent of EventReceiver implementation)
irr::gui::IGUIWindow* createChooseFromListDialog(irr::IrrlichtDevice* device, std::vector<irr::gui::IGUIButton*>& buttonsOut, const std::vector<int32_t>& buttonIds, const std::vector<std::wstring>& buttonLabels, const std::wstring& headline, int32_t aggregationId, int32_t aggregatableId, bool modal = true);

//! returns the selected index (dependent on EventReceiver implementation)
uint32_t startChooseFromListDialog(irr::IrrlichtDevice* device, const std::vector<int32_t>& buttonIds, const std::vector<std::wstring>& buttonLabels, const std::wstring& headline, int32_t aggregationId, int32_t aggregatableId, irr::video::SColor bgColor, bool modal = true);

#endif
