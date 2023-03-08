#include <IFileSystem.h>
#include <IReadFile.h>

#include <IniFile.h>

#include "utilities.h"

#include "mathUtils.h"

#include <IGUIElement.h>
#include <IImage.h>
#include <IMesh.h>
#include <IVideoDriver.h>
#include <IrrlichtDevice.h>
#include <IGUIFont.h>

#include <iostream>

using namespace irr;
using namespace core;
using namespace video;
using namespace io;
using namespace gui;

irr::core::rect<irr::s32> limitRect(irr::core::rect<irr::s32> r, const irr::core::rect<irr::s32>& limiting){
	//resize if to big for limiting
	int dw = r.getWidth()-limiting.getWidth(), dh = r.getHeight()-limiting.getHeight();
	if(dw>0){
		r.UpperLeftCorner.X += dw/2;
		r.LowerRightCorner.X -= dw/2;
	}else if(dh>0){
		r.UpperLeftCorner.Y += dh/2;
		r.LowerRightCorner.Y -= dh/2;
	}
	//move when outside limiting
	vector2d<s32> dup = r.UpperLeftCorner-limiting.UpperLeftCorner;
	vector2d<s32> dlow = r.LowerRightCorner-limiting.LowerRightCorner;
	if(dup.X<0){
		r.UpperLeftCorner.X -= dup.X;
		r.LowerRightCorner.X -= dup.X;
	}
	if(dlow.X>0){
		r.UpperLeftCorner.X -= dlow.X;
		r.LowerRightCorner.X -= dlow.X;
	}
	if(dup.Y<0){
		r.UpperLeftCorner.Y -= dup.Y;
		r.LowerRightCorner.Y -= dup.Y;
	}
	if(dlow.Y>0){
		r.UpperLeftCorner.Y -= dlow.Y;
		r.LowerRightCorner.Y -= dlow.Y;
	}
	return r;
}

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

bool loadFileWithAssetSupport(irr::io::IFileSystem* fsys, const std::string& file, char*& bufferOut, uint32_t& bufferSizeOut){
	IReadFile* rf = fsys->createAndOpenFile(file.c_str());
	if(rf){
		bufferSizeOut = rf->getSize();
		bufferOut = new char[bufferSizeOut];
		rf->read(bufferOut, bufferSizeOut);
		rf->drop();
		return true;
	}else{
		std::cerr << "Unable to load file: " << file << std::endl;
	}
	bufferOut = NULL;
	bufferSizeOut = 0;
	return false;
}

std::function<bool(char*&, uint32_t&)> createLoadFileFunction(irr::io::IFileSystem* fsys, const std::string& file){
	return [fsys, file](char*& bufferOut, uint32_t& bufferSizeOut){
		return loadFileWithAssetSupport(fsys, file, bufferOut, bufferSizeOut);
	};
}

bool loadFileWithAssetSupportIntoVector(irr::io::IFileSystem* fsys, const std::string& file, std::vector<char>& v){
	IReadFile* rf = fsys->createAndOpenFile(file.c_str());
	if(rf){
		uint32_t size = rf->getSize();
		v.resize(size);
		rf->read(&(v[0]), size);
		rf->drop();
		return true;
	}
	return false;
}

std::function<bool(std::vector<char>&)> createLoadFileIntoVectorFunction(irr::io::IFileSystem* fsys, const std::string& file){
	return [fsys, file](std::vector<char>& v){
		return loadFileWithAssetSupportIntoVector(fsys, file, v);
	};
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
