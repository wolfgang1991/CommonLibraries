#ifndef NumberEditBox_H_INCLUDED
#define NumberEditBox_H_INCLUDED

#include <IGUIElement.h>
#include <IGUIStaticText.h>
#include <IGUIButton.h>

//! Editbox for Integer Numbers with small and large steps
class NumberEditBox : public irr::gui::IGUIElement{

	private:
	
	irr::gui::IGUIEnvironment* env;
	
	irr::s32 minValue, maxValue;
	irr::s32 largeStep, normalStep;
	
	irr::s32 value;
	
	irr::gui::IGUIStaticText* etext;
	irr::gui::IGUIButton* ldbut;
	irr::gui::IGUIButton* dbut;
	irr::gui::IGUIButton* libut;
	irr::gui::IGUIButton* ibut;

	public:

	NumberEditBox(irr::gui::IGUIEnvironment* env, irr::gui::IGUIElement* parent, irr::s32 id, const irr::core::rect<irr::s32> &rectangle, irr::s32 minValue, irr::s32 maxValue, irr::s32 largeStep, irr::s32 normalStep, irr::s32 defaultValue, irr::s32 buttonWidth);
	
	~NumberEditBox();
	
	irr::s32 getValue() const;
	
	void setValue(irr::s32 value);
	
	irr::gui::IGUIStaticText* getStaticText() const;
	
	irr::gui::IGUIButton* getLargeDecrementButton() const;
	
	irr::gui::IGUIButton* getDecrementButton() const;
	
	irr::gui::IGUIButton* getLargeIncrementButton() const;
	
	irr::gui::IGUIButton* getIncrementButton() const;
	
	const wchar_t* getText() const;
	
	void setText(const wchar_t* text);
	
	bool OnEvent(const irr::SEvent& event);

};

//! similar to add... methods for built in gui elements in IGUIEnvironment
NumberEditBox* addNumberEditBox(irr::gui::IGUIEnvironment* env, irr::gui::IGUIElement* parent, const irr::core::rect<irr::s32> &rectangle, irr::s32 buttonWidth, irr::s32 minValue, irr::s32 maxValue, irr::s32 largeStep, irr::s32 normalStep, irr::s32 defaultValue, irr::s32 id = -1);

#endif
