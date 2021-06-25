#ifndef GUIHELP_H_INCLUDED
#define GUIHELP_H_INCLUDED

#include <RectangleGradientDescent.h>

#include <IEventReceiver.h>
#include <CMeshBuffer.h>

#include <string>
#include <list>
#include <unordered_map>

class IGUIDefinition;
class FlexibleFont;
class Drawer2D;
class UnicodeCfgParser;

#include "ForwardDeclarations.h"

class GUIHelp : public irr::IEventReceiver{

	private:
	
	struct Bubble{
		irr::gui::IGUIElement* ele;
		irr::core::rect<irr::s32> alternativeRectangle;//! in case ele is NULL
		double orientation;
		double width;
		double padding;
		std::wstring text;
		bool showLine;
		bool isVisible;//truly visible <=> isVisible && (!ele || ele->isVisible)
		
		irr::core::dimension2d<irr::u32> dim;
		RectangleGradientDescent::RectanglePair* rgdRect;
		
		irr::scene::SMeshBuffer bubbleMb, textMb;
		
		Bubble(irr::gui::IGUIElement* ele, double orientation, double width, double padding, const std::wstring& text, Drawer2D* drawer, FlexibleFont* font, irr::video::ITexture* bubble, irr::f32 cornerSize, irr::f32 realCornerSize, irr::video::SColor fontColor, bool showLine);
		
	};
	
	std::list<Bubble> bubbles;
	std::unordered_map<std::string, Bubble*> id2bubble;
	ConcurrentRectangleGradientDescent crgd;
	
	IGUIDefinition* gui;
	FlexibleFont* font;
	Drawer2D* drawer;
	irr::video::IVideoDriver* driver;
	
	irr::video::ITexture* bubble;
	irr::f32 cornerSize, realCornerSize;
	irr::video::SColor fontColor;

	irr::s32 lineWidth;

	bool visible;
	
	void initHelpFromCfgResult(const UnicodeCfgParser& parser);

	public:
	
	//! cornersize in texture coordinates (part of width/height) (e.g. 0.15f), realCornerSize in pixels on the screen
	GUIHelp(IGUIDefinition* gui, FlexibleFont* font, Drawer2D* drawer, irr::video::ITexture* bubble, irr::f32 cornerSize, irr::f32 realCornerSize, irr::s32 lineWidth, irr::video::SColor fontColor =  irr::video::SColor(255,0,0,0));
	
	~GUIHelp();
	
	//! help Syntax: sequence of <element_id>,<initial_bubble_orientation_[0-360]>,<max_width_of_bubble_on_screen_[0-1]>,<+/-_padding_[-1-1]>,<show_line_to_element_0|1>,<help_text>;
	//! escape character: '\'; characters to escape: \\ \, \; \n (\n == new line)
	void initHelp(const std::wstring& help);
	
	void initHelpFromFile(irr::io::IFileSystem* fsys, const char* path);
	
	void startGradientDescent();
	
	void render();
	
	bool OnEvent(const irr::SEvent& event);
	
	void setVisible(bool visible);
	
	bool isVisible();
	
	void setBubbleVisible(const std::string& id, bool visible);
	
	void setBubbleOrientation(const std::string& id, double orientation);
	
};

#endif
