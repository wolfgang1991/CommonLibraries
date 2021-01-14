#include <GUIHelp.h>
#include <StringHelpers.h>
#include <Drawer2D.h>
#include <GUI.h>
#include <font.h>
#include <FlexibleFont.h>
#include <mathUtils.h>
#include <UnicodeCfgParser.h>

#include <IrrlichtDevice.h>
#include <irrArray.h>
#include <IGUIElement.h>
#include <cassert>

#include <iostream>

using namespace irr;
using namespace core;
using namespace video;
using namespace scene;
using namespace gui;

#define AVOID_PIVOT_RECTANGLES false

static rect<double> initSpace(IrrlichtDevice* device){
	dimension2d<u32> dim = device->getVideoDriver()->getScreenSize();
	return rect<double>(0,0,dim.Width,dim.Height);
}

GUIHelp::GUIHelp(GUI* gui, FlexibleFont* font, Drawer2D* drawer, irr::video::ITexture* bubble, irr::f32 cornerSize, irr::f32 realCornerSize, irr::s32 lineWidth, irr::video::SColor fontColor):
	crgd(initSpace(drawer->getDevice())),
	gui(gui),
	font(font),
	drawer(drawer),
	bubble(bubble),
	cornerSize(cornerSize),
	realCornerSize(realCornerSize),
	fontColor(fontColor),
	lineWidth(lineWidth),
	visible(true){
	driver = drawer->getDevice()->getVideoDriver();
}

GUIHelp::~GUIHelp(){
	for(auto it=bubbles.begin(); it!=bubbles.end(); ++it){
		driver->removeHardwareBuffer(&(it->bubbleMb));
		driver->removeHardwareBuffer(&(it->textMb));
	}
}

void GUIHelp::initHelpFromCfgResult(const UnicodeCfgParser& parser){
	const std::list<std::vector<std::wstring>>& result = parser.getResult();
	dimension2d<u32> dim = drawer->getDevice()->getVideoDriver()->getScreenSize();
	for(auto it = result.begin(); it != result.end(); ++it){
		const std::vector<std::wstring>& line = *it;
		std::string id = convertWStringToString(line[0]);
		IGUIElement* ele = gui->getElement(id);
		double orientation = convertWStringTo<double>(line[1]);
		double width = convertWStringTo<double>(line[2])*dim.Width;
		double padding = convertWStringTo<double>(line[3]);
		bool showLine = (bool)convertWStringTo<int>(line[4]);
		const std::wstring& helpText = line[5];
		bubbles.push_back(Bubble(ele, orientation, width, padding, makeWordWrappedText(helpText, width, font), drawer, font, bubble, cornerSize, realCornerSize, fontColor, showLine));
		id2bubble[id] = &(bubbles.back());
		if(!ele){
			bubbles.back().alternativeRectangle = gui->getSpecialRectangle(id);
			if(bubbles.back().alternativeRectangle.getArea()==0){
				std::cerr << "No GUI Element and no rectangle found: " << id << std::endl;
			}
		}
	}
}

void GUIHelp::initHelp(const std::wstring& help){
	UnicodeCfgParser parser(6);
	parser.parse(help);
	initHelpFromCfgResult(parser);
}

void GUIHelp::initHelpFromFile(irr::io::IFileSystem* fsys, const char* path){
	UnicodeCfgParser parser(6);
	parser.parseFromUTF8File(fsys, path);
	initHelpFromCfgResult(parser);
}

void GUIHelp::startGradientDescent(){
	crgd.abort();
	RectangleGradientDescent* rgd = crgd.getRectangleGradientDescent();
	rgd->clear();
	for(auto it=bubbles.begin(); it!=bubbles.end(); ++it){
		rect<s32> absPos = it->ele?(it->ele->getAbsolutePosition()):(it->alternativeRectangle);
		vector2d<s32> pivot = absPos.getCenter();
		it->rgdRect = rgd->addRectangle(RectangleGradientDescent::Rectangle{vector2d<double>(pivot.X,pivot.Y), (double)it->dim.Width, (double)it->dim.Height, (double)it->orientation/DEG_RAD, (1.0+it->padding)*(double)absPos.getWidth(), (1.0+it->padding)*(double)absPos.getHeight(), &rgd->getSpaceLimit()});//TODO use dim with corner size
	}
	crgd.start(400, 1.0/DEG_RAD, 0.01, AVOID_PIVOT_RECTANGLES);
}

void GUIHelp::render(){
	if(visible && !crgd.isRunning()){
		for(auto it=bubbles.begin(); it!=bubbles.end(); ++it){
			auto rgdRect = it->rgdRect;
			if(rgdRect){
				if(!it->isVisible){continue;}
				IGUIElement* ele = it->ele;
				if(ele){
					if(!ele->isVisible()){continue;}
				}
				rect<double>& r = rgdRect->second;
				vector2d<double> ul = r.UpperLeftCorner;
				drawer->draw2DMeshBuffer(&it->bubbleMb, vector2d<f32>(ul.X, ul.Y));
				rect<s32> s32Rect(r.UpperLeftCorner.X, r.UpperLeftCorner.Y, r.LowerRightCorner.X, r.LowerRightCorner.Y);
				font->drawFontMeshBuffer(it->textMb, it->dim, s32Rect, false, false, NULL);
				if(it->showLine){
					vector2d<s32> start = s32Rect.getCenter();
					vector2d<s32> end(rgdRect->first.pivot.X, rgdRect->first.pivot.Y);
					s32 offset = realCornerSize+lineWidth/2;
					if(end.Y>s32Rect.LowerRightCorner.Y){
						start.Y = s32Rect.LowerRightCorner.Y+offset;
					}else if(end.Y<s32Rect.UpperLeftCorner.Y){
						start.Y = s32Rect.UpperLeftCorner.Y-offset;
					}
					if(end.X>s32Rect.LowerRightCorner.X){
						start.X = s32Rect.LowerRightCorner.X+offset;
					}else if(end.X<s32Rect.UpperLeftCorner.X){
						start.X = s32Rect.UpperLeftCorner.X-offset;
					}
					drawer->setColor(SColor(255,0,0,0));
					drawer->drawLine(NULL, start, end, lineWidth);
					drawer->setColor(SColor(255,255,255,255));
				}
			}
		}
	}
}

template <typename T>
static void moveToBack(std::list<T>& list, typename std::list<T>::iterator it){
	list.splice(list.end(), list, it, std::next(it));
}

bool GUIHelp::OnEvent(const irr::SEvent& event){
	if(visible){
		if(event.EventType==EET_MOUSE_INPUT_EVENT && !crgd.isRunning()){
			const SEvent::SMouseInput& m = event.MouseInput;
			vector2d<double> mPos(m.X, m.Y);
			for(auto it=bubbles.begin(); it!=bubbles.end(); ++it){
				auto rgdRect = it->rgdRect;
				if(rgdRect){
					rect<double>& r = rgdRect->second;
					if(r.isPointInside(mPos)){
						if(m.Event==EMIE_LMOUSE_LEFT_UP){moveToBack(bubbles, it);}
						return true;
					}
				}
			}
		}
	}
	return false;
}

void GUIHelp::setVisible(bool visible){
	this->visible = visible;
}

bool GUIHelp::isVisible(){
	return visible;
}

void GUIHelp::setBubbleVisible(const std::string& id, bool visible){
	auto it = id2bubble.find(id);
	assert(it!=id2bubble.end());
	it->second->isVisible = visible;
}

void GUIHelp::setBubbleOrientation(const std::string& id, double orientation){
	auto it = id2bubble.find(id);
	assert(it!=id2bubble.end());
	it->second->orientation = orientation;
	if(!crgd.isRunning()){
		RectangleGradientDescent::RectanglePair* rgdRect = it->second->rgdRect;
		if(rgdRect){
			rgdRect->first.angle = orientation/DEG_RAD;
			rgdRect->second = rgdRect->first.convertToRect();
		}
	}
}

GUIHelp::Bubble::Bubble(irr::gui::IGUIElement* ele, double orientation, double width, double padding, const std::wstring& text, Drawer2D* drawer, FlexibleFont* font, irr::video::ITexture* bubble, irr::f32 cornerSize, irr::f32 realCornerSize, irr::video::SColor fontColor, bool showLine):
	ele(ele),
	alternativeRectangle(0,0,0,0),
	orientation(orientation),
	width(width),
	padding(padding),
	text(text),
	showLine(showLine),
	isVisible(true){
	dim = font->getDimension(text.c_str());
	rgdRect = NULL;
	bubbleMb.setHardwareMappingHint(EHM_STATIC);
	textMb.setHardwareMappingHint(EHM_STATIC);
	set2DMaterialParams(bubbleMb.Material, ETC_CLAMP, ETC_CLAMP, EMT_TRANSPARENT_ALPHA_CHANNEL);
	drawer->fillRectWithCornerMeshBuffer(&bubbleMb, rect<f32>(0,0,dim.Width,dim.Height), cornerSize, realCornerSize, bubble, SColor(255,255,255,255));
	font->fillMeshBuffer(textMb, text.c_str(), 4, false, fontColor, 0.f, NULL, font->getDefaultMaterialType());
}
