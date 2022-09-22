#ifndef EditBoxDialog_H_INCLUDED
#define EditBoxDialog_H_INCLUDED

#include <IGUIElement.h>

#include "ForwardDeclarations.h"

#include <string>
#include <functional>

class EditBoxDialog;

class IEditBoxDialogCallback{

	public:
	
	virtual ~IEditBoxDialogCallback(){}
	
	//! positivePressed true if ok false if cancel pressed
	virtual void OnResult(EditBoxDialog* dialog, const wchar_t* text, bool positivePressed) = 0;

};

//! Deprecated use lambda constructor instead in EditBoxDialog
class LambdaEditBoxCallback : public IEditBoxDialogCallback{

	public:
	
	std::function<void(EditBoxDialog*, const wchar_t*, bool)> f;
	
	LambdaEditBoxCallback(const std::function<void(EditBoxDialog*, const wchar_t*, bool)>& f):f(f){}
	
	virtual ~LambdaEditBoxCallback(){}
	
	virtual void OnResult(EditBoxDialog* dialog, const wchar_t* text, bool positivePressed){
		f(dialog,text,positivePressed);
	}
};

//! deletes itself after it has been closed by the user
class EditBoxDialog : public irr::gui::IGUIElement{

	private:
	
	irr::IrrlichtDevice* device;
	
	IEditBoxDialogCallback* cbk;
	std::function<void(EditBoxDialog* dialog, const wchar_t* text, bool positivePressed)> onResult;
	
	GUI* gui;
	
	void init(const wchar_t* title, const wchar_t* positiveLabel, const wchar_t* negativeLabel, const wchar_t* defaultText, bool isModal, bool isPasswordBox, wchar_t passwordChar, irr::core::rect<irr::s32>* spaceOverride);
	
	public:

	irr::gui::IGUIWindow* win;
	irr::gui::IGUIStaticText* ettitle;
	irr::gui::IGUIEditBox* eeedit;
	irr::gui::IGUIButton* ebok;
	irr::gui::IGUIButton* ebcancel;
	
	
	//! the .*AggId parameters define the ids for the skin for the aggregation
	//! spaceOverride: screenArea which is available for the editbox. If not specified a default area is used
	EditBoxDialog(IEditBoxDialogCallback* cbk, irr::IrrlichtDevice* device, const wchar_t* title, const wchar_t* positiveLabel = L"OK", const wchar_t* negativeLabel = L"Cancel", const wchar_t* defaultText = L"", bool isModal = true, bool isPasswordBox = false, wchar_t passwordChar = L'·', irr::core::rect<irr::s32>* spaceOverride = NULL);
	
	//! spaceOverride: screenArea which is available for the editbox. If not specified a default area is used
	EditBoxDialog(const std::function<void(EditBoxDialog*, const wchar_t*, bool)>& onResult, irr::IrrlichtDevice* device, const wchar_t* title, const wchar_t* positiveLabel = L"OK", const wchar_t* negativeLabel = L"Cancel", const wchar_t* defaultText = L"", bool isModal = true, bool isPasswordBox = false, wchar_t passwordChar = L'·', irr::core::rect<irr::s32>* spaceOverride = NULL);
	
	~EditBoxDialog();
	
	bool OnEvent(const irr::SEvent& event);
	
	//! resizeFactor: multiplies the editbox height (should be >= 1), should only be called once
	void enableMultLineEditing(irr::f32 resizeFactor = 1.f);
	
	irr::gui::IGUIEditBox* getEditBox() const{return eeedit;}
	
};

//! returns true if ok pressed
bool startEditBoxDialog(std::wstring& out, irr::video::SColor bgColor, irr::IrrlichtDevice* device, const wchar_t* title, const wchar_t* positiveLabel = L"Ok", const wchar_t* negativeLabel = L"Cancel", const wchar_t* defaultText = L"", bool isModal = true, bool isPasswordBox = false, wchar_t passwordChar = L'·', const std::function<void()>& updateFunc = [](){});

#endif
