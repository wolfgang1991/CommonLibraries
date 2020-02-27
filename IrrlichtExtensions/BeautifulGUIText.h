#ifndef BeautifulGUIText_H_INCLUDED
#define BeautifulGUIText_H_INCLUDED

#include "IAggregatableGUIElement.h"
#include "ForwardDeclarations.h"

#include <CMeshBuffer.h>

class FlexibleFont;

//! asserts that the current font in use can safely be casted to FlexibleFont
//! the meshbuffer is automatically recalculated if the font or the meshbuffer parameters change
//! it uses the sdff materialtype with the default parameters of the current font
class BeautifulGUIText : public IAggregatableGUIElement{

	private:
	
	std::wstring text;
	irr::video::SColor color;
	irr::f32 italicGradient;
	irr::core::matrix4 transformation;
	bool useTransformation;
	
	irr::scene::SMeshBuffer mb;
	FlexibleFont* font;
	
	irr::core::dimension2d<irr::u32> textSize;
	bool hcenter;
	bool vcenter;
	
	irr::video::IVideoDriver* driver;
	
	irr::f32 scale;
	
	public:
	
	//! test must already be broken, transformation may be NULL for unit transformation, scale used for rendering: defaultScale*scale
	BeautifulGUIText(const wchar_t* text, irr::video::SColor color, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id = -1, irr::f32 scale = 1.f, void* data = NULL, IGUIElement* parent = NULL, const irr::core::rect<irr::s32>& rectangle = irr::core::rect<irr::s32>(0,0,0,0));
	
	~BeautifulGUIText();
	
	void draw();
	
	//! recalculates the meshbuffer using the current font
	void recalculateMeshBuffer();
	
	void setColor(irr::video::SColor color);
	
	void setItalicGradient(irr::f32 italicGradient);
	
	void setCenter(bool hcenter, bool vcenter);
	
	void setText(const wchar_t* text);
	
	irr::scene::SMeshBuffer& getMeshBuffer();
	
};

#endif
