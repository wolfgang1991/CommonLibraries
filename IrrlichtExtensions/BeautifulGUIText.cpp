#include <BeautifulGUIText.h>
#include <FlexibleFont.h>

#include <IGUIEnvironment.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace gui;
using namespace scene;

static const irr::video::SColor white(255,255,255,255);

BeautifulGUIText::BeautifulGUIText(const wchar_t* text, irr::video::SColor color, irr::f32 italicGradient, irr::core::matrix4* transformation, bool hcenter, bool vcenter, irr::gui::IGUIEnvironment* environment, irr::f32 recommendedSpace, irr::s32 id, irr::f32 scale, void* data, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle):
	IAggregatableGUIElement(environment, recommendedSpace, 1.f, recommendedSpace, 1.f, false, false, id, data, parent, rectangle),
	text(text),
	color(color),
	italicGradient(italicGradient),
	transformation(transformation?*transformation:matrix4()),
	useTransformation(transformation!=NULL),
	font(NULL),
	hcenter(hcenter),
	vcenter(vcenter),
	scale(scale){
	setName("BeautifulGUIText");
	mb.setHardwareMappingHint(EHM_STATIC);
	recalculateMeshBuffer();
	driver = environment->getVideoDriver();
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
	FlexibleFont* guiFont = (FlexibleFont*)(Environment->getSkin()->getFont());
	if(guiFont!=font){font = guiFont;}
	mb.Vertices.clear();
	mb.Indices.clear();
	font->fillMeshBuffer(mb, text.c_str(), font->getDefaultTabSize(), hcenter, color, italicGradient, useTransformation?&transformation:NULL, font->getFontManager()->getSDFFMaterialType(), true, true);
	textSize = get2DDimensionFromMeshBuffer(mb);
}

void BeautifulGUIText::setCenter(bool hcenter, bool vcenter){
	this->hcenter = hcenter;
	this->vcenter = vcenter;
	recalculateMeshBuffer();
}

BeautifulGUIText::~BeautifulGUIText(){
	driver->removeHardwareBuffer(&mb);
}

void BeautifulGUIText::setText(const wchar_t* text){
	font = NULL;
	this->text = text;
}

irr::scene::SMeshBuffer& BeautifulGUIText::getMeshBuffer(){
	return mb;
}
