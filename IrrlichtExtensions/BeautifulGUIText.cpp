#include <BeautifulGUIText.h>
#include <FlexibleFont.h>

#include <IGUIEnvironment.h>

#include <set>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;
using namespace scene;

static std::set<BeautifulGUIText*> textWithStandardColor;

void setAllBeautifulTextStandardColor(irr::video::SColor color){
	for(BeautifulGUIText* text : textWithStandardColor){
		text->setColor(color);
	}
}

static const irr::video::SColor white(255,255,255,255);

BeautifulGUIText::BeautifulGUIText(const wchar_t* text, irr::video::SColor color, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id, irr::f32 scale, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, 1.f, recommendedSpace, 1.f, false, false, id, data, parent, rectangle),
	color(color),
	italicGradient(italicGradient),
	transformation(transformation?*transformation:matrix4()),
	useTransformation(transformation!=NULL),
	font(NULL),
	hcenter(hcenter),
	vcenter(vcenter),
	scale(scale){
	setText(text);
	setName("BeautifulGUIText");
	mb.setHardwareMappingHint(EHM_STATIC);
	recalculateMeshBuffer();
	driver = environment->getVideoDriver();
}

BeautifulGUIText::BeautifulGUIText(const wchar_t* text, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id, irr::f32 scale, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, 1.f, recommendedSpace, 1.f, false, false, id, data, parent, rectangle),
	color(environment->getSkin()->getColor(EGDC_BUTTON_TEXT)),
	italicGradient(italicGradient),
	transformation(transformation?*transformation:matrix4()),
	useTransformation(transformation!=NULL),
	font(NULL),
	hcenter(hcenter),
	vcenter(vcenter),
	scale(scale){
	setText(text);
	setName("BeautifulGUIText");
	mb.setHardwareMappingHint(EHM_STATIC);
	recalculateMeshBuffer();
	driver = environment->getVideoDriver();
	textWithStandardColor.insert(this);
}

void BeautifulGUIText::setColor(irr::video::SColor color){
	this->color = color;
	recalculateMeshBuffer();
}

void BeautifulGUIText::setItalicGradient(irr::f32 italicGradient){
	this->italicGradient = italicGradient;
	recalculateMeshBuffer();
}
	
void BeautifulGUIText::draw(){
	if(isVisible()){
		FlexibleFont* guiFont = (FlexibleFont*)(Environment->getSkin()->getFont());
		if(guiFont!=font){
			recalculateMeshBuffer();
		}
		auto cScale = guiFont->getDefaultScale();
		guiFont->setDefaultScale(scale*cScale);
		guiFont->drawFontMeshBuffer(mb, textSize, AbsoluteRect, hcenter, vcenter, &AbsoluteClippingRect);
		guiFont->setDefaultScale(cScale);
		IAggregatableGUIElement::draw();//draw children etc
	}
}

void BeautifulGUIText::recalculateMeshBuffer(){
	font = (FlexibleFont*)(Environment->getSkin()->getFont());
	mb.Vertices.clear();
	mb.Indices.clear();
	font->fillMeshBuffer(mb, getText(), font->getDefaultTabSize(), hcenter, color, italicGradient, useTransformation?&transformation:NULL, font->getFontManager()->getSDFFMaterialType(), true, true);
	textSize = font->getDimensionWithTabs(getText(), font->getDefaultTabSize());//get2DDimensionFromMeshBuffer(mb);
}

void BeautifulGUIText::setCenter(bool hcenter, bool vcenter){
	this->hcenter = hcenter;
	this->vcenter = vcenter;
	recalculateMeshBuffer();
}

BeautifulGUIText::~BeautifulGUIText(){
	driver->removeHardwareBuffer(&mb);
	textWithStandardColor.erase(this);
}

void BeautifulGUIText::setText(const wchar_t* text){
	font = NULL;
	IAggregatableGUIElement::setText(text);
}

irr::scene::SMeshBuffer& BeautifulGUIText::getMeshBuffer(){
	return mb;
}
