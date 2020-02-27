#include <IFileSystem.h>
#include <IReadFile.h>

#include <IniFile.h>

#include "utilities.h"

#include <mathUtils.h>

#include <IGUIElement.h>
#include <IImage.h>
#include <IMesh.h>
#include <IVideoDriver.h>
#include <IrrlichtDevice.h>
#include <IGUIFont.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace io;
using namespace gui;

void readIniWithAssetSupport(irr::io::IFileSystem* fsys, const std::string& file, IniFile& ini){
	IReadFile* rf = fsys->createAndOpenFile(file.c_str());
	if(rf){
		uint32_t size = rf->getSize();
		char* fontStr = new char[size+1];
		fontStr[size] = '\0';
		rf->read(fontStr, size);
		rf->drop();
		ini.setFromString(fontStr);
		delete[] fontStr;
	}
}

bool isOverlayedBySingleGUIElement(irr::gui::IGUIElement* ele, const irr::core::rect<s32>& rectangle){
	if(!ele->isTrulyVisible()){return false;}
	return ele->getAbsolutePosition().isRectCollided(rectangle);
}

//! checks all children recursively
bool isOverlayedByGUIElements(irr::gui::IGUIElement* root, const irr::core::rect<s32>& rectangle){
	auto children = root->getChildren();
	for(auto it = children.begin(); it != children.end(); ++it){
		IGUIElement* ele = *it;
		if(ele->isVisible()){
			if(ele->getAbsolutePosition().isRectCollided(rectangle)){return true;}
			if(isOverlayedByGUIElements(ele, rectangle)){return true;}
		}
	}
	return false;
}

bool isOverlayedBySingleGUIElement(irr::gui::IGUIElement* ele, const irr::core::vector2d<irr::s32>& pos){
	if(!ele->isTrulyVisible()){return false;}
	return ele->getAbsolutePosition().isPointInside(pos);
}

bool isOverlayedByGUIElements(irr::gui::IGUIElement* root, const irr::core::vector2d<irr::s32>& pos){
	auto children = root->getChildren();
	for(auto it = children.begin(); it != children.end(); ++it){
		IGUIElement* ele = *it;
		if(ele->isVisible()){
			if(ele->getAbsolutePosition().isPointInside(pos)){return true;}
			if(isOverlayedByGUIElements(ele, pos)){return true;}
		}
	}
	return false;
}

irr::video::SColor getInterpolatedColor(irr::video::IImage* img, double sx, double sy){
	int px[4], py[4]; double d[4];
	px[0] = (int)sx; py[0] = (int)sy;
	px[1] = px[0]+1; py[1] = py[0];
	px[2] = px[0]+1; py[2] = py[0]+1;
	px[3] = px[0]; py[3] = py[0]+1;
	int w = img->getDimension().Width, h = img->getDimension().Height;
	for(int i=0; i<4; i++){
		d[i] = sqrt(sq(sx-(double)px[i])+sq(sy-(double)py[i]));
	}
	double sumWeight = 0.0;
	double sumA = 0, sumR = 0, sumG = 0, sumB = 0;
	for(int i=0; i<4; i++){
		if(px[i]>=0 && px[i]<w && py[i]>=0 && py[i]<h){
			SColor color = img->getPixel(px[i], py[i]);
			double g = 1.0;
			for(int j=0; j<4; j++){
				if(i!=j){g *= d[j];}
			}
			sumWeight += g;
			sumA += g*(double)(color.getAlpha());
			sumR += g*(double)(color.getRed());
			sumG += g*(double)(color.getGreen());
			sumB += g*(double)(color.getBlue());
		}
	}
	return SColor((u32)(sumA/sumWeight), (u32)(sumR/sumWeight), (u32)(sumG/sumWeight), (u32)(sumB/sumWeight));
}

void removeMeshHardwareBuffers(irr::video::IVideoDriver* driver, irr::scene::IMesh* mesh){
	for(u32 i=0; i<mesh->getMeshBufferCount(); i++){
		irr::scene::IMeshBuffer* mb = mesh->getMeshBuffer(i);
		driver->removeHardwareBuffer(mb);
	}
}

bool showState(IrrlichtDevice* device, const wchar_t* txt){
	bool res = device->run();
	if(device->isWindowActive()){
		IVideoDriver* driver = device->getVideoDriver();
		dimension2d<u32> dim = driver->getScreenSize();
		driver->beginScene(true, true, SColor(255, 0, 0, 0));
		device->getGUIEnvironment()->getSkin()->getFont()->draw(txt, rect<s32>(0,0,dim.Width,dim.Height), SColor(255,255,255,255), true, true);
		driver->endScene();
	}
	return res;
}
