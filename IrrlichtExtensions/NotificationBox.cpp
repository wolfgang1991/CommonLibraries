#include <NotificationBox.h>

#include <IGUIButton.h>

#include <sstream>

using namespace irr;
using namespace gui;

static const irr::u32 invalidTime = ~(irr::u32)0;

NotificationBox::NotificationBox(irr::u32 timeoutMs, irr::IrrlichtDevice* device, std::wstring text, irr::f32 maxW, irr::f32 maxH , const wchar_t* positive, const wchar_t* negative, irr::s32 posId, irr::s32 negId, bool modal, irr::gui::EGUI_ALIGNMENT horizontal, irr::gui::EGUI_ALIGNMENT vertical):
	CMBox(device, text, maxW, maxH, positive, negative, posId, negId, modal, horizontal, vertical),timeoutMs(timeoutMs),startMs(invalidTime),label(positive?positive:(negative?negative:L"")){
}

void NotificationBox::OnPostRender(irr::u32 timeMs){
	CMBox::OnPostRender(timeMs);
	if(startMs==invalidTime){
		startMs = timeMs;
	}
	irr::u32 deltaT = timeMs-startMs;
	if(deltaT>timeoutMs){
		if(pos && !mustDelete){
			SEvent toPost;
			toPost.EventType = EET_GUI_EVENT;
			toPost.GUIEvent.Caller = this;
			toPost.GUIEvent.Element = pos;
			toPost.GUIEvent.EventType = EGET_MESSAGEBOX_YES;
			setVisible(false);
			grab();//in case it gets removed in OnEvent (e.g. if the Parent decides to delete)
			Parent->OnEvent(toPost);
			onPositive();
			drop();
		}
		mustDelete = true;
	}else{
		irr::u32 remainingSecs = (timeoutMs-deltaT+500)/1000;
		std::wstringstream ss; ss << label << L" (" << remainingSecs << L")";
		if(pos){
			pos->setText(ss.str().c_str());
		}else if(neg){
			neg->setText(ss.str().c_str());
		}
	}
}
