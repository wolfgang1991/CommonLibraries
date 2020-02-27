#include <FlexibleFont.h>
#include <Drawer2D.h>
#include <timing.h>
#include <Transformation2DHelpers.h>
#include <utilities.h>
#include <mathUtils.h>

#include <irrlicht.h>

#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

int main(int argc, char *argv[]){
	IrrlichtDevice* nulldevice = createDevice(EDT_NULL);
	core::dimension2d<u32> deskres = nulldevice->getVideoModeList()->getDesktopResolution();
	nulldevice->drop();
	
	SIrrlichtCreationParameters param;
	param.Bits = 32;
   param.DriverType = EDT_OPENGL;
	rect<irr::s32> winRect(0,0,9*deskres.Width/10,9*deskres.Height/10);
	param.WindowSize = winRect.getSize();
	
	IrrlichtDevice* device = createDeviceEx(param);
	IVideoDriver* driver = device->getVideoDriver();
	IGUIEnvironment* env = device->getGUIEnvironment();
	ISceneManager* smgr = device->getSceneManager();
	
	device->setWindowCaption(L"Manual Tests for Font Stuff");
	
	FlexibleFontManager* fmgr = new FlexibleFontManager(device);
	BMFontLoader* bmFontLoader = new BMFontLoader(fmgr);
	fmgr->addFontLoader(bmFontLoader);
	bmFontLoader->drop();
	
	FlexibleFont* bitmapFont = fmgr->getFont("roboto-medium-bitmap.fnt");
	
	FlexibleFont* ff = fmgr->getFont("roboto-medium.fnt");
	//fmgr->removeFont(ff);//ok but drop not ok
	ff = fmgr->getFont("roboto-medium.fnt");//don't drop a font since it is cached. Reusing it this way is ok.
	//ff->setDefaultMaterialType(fmgr->getSDFFMaterialType());
	//ff->setDefaultMaterialType(fmgr->getSDFFWithShadowsMaterialType());
	//ff->setDefaultScale(vector2df(3.f,3.f));
	ff->setDefaultBorderColor(SColor(255,255,128,0));
	//ff->setDefaultBorderSize(0.05f);
	//ff->setDefaultSmoothingFactor(1.5f);
	//ff->setDefaultItalicGradient(0.5f);
	//ff->setDefaultFatness(0.2f);
	env->getSkin()->setFont(ff);
	
	FlexibleFont* chineseFont = fmgr->getFont("SomeChineseGlyphs.fnt");
	chineseFont->setDefaultMaterialType(fmgr->getSDFFMaterialType());//fmgr->getSDFFWithShadowsMaterialType());
	chineseFont->setDefaultFatness(.45f);
	//chineseFont->setDefaultBorderSize(0.01f);
	
	SMeshBuffer testBuffer;
	stringw testString = L"Test\tBla\nt\töäüß?#+|€\na\tb\n\ta";//L"Error: No machine\nselected.";//
	ff->fillMeshBuffer(testBuffer, testString.c_str(), 4, false, SColor(255,255,0,0), .0f);
	testBuffer.setHardwareMappingHint(EHM_STATIC);
	dimension2d<u32> testDim = ff->getDimensionWithTabs(testString.c_str());
	//dimension2d<u32> testDim = get2DDimensionFromMeshBuffer(testBuffer);
	std::cout << "testDim: " << testDim.Width << ", " << testDim.Height << std::endl;
	
	stringw addedString = L"added";
	dimension2d<u32> addedDim = ff->getDimensionWithTabs(addedString.c_str());
	matrix4 T = create2DWorldTransformation(vector2d<f32>(100,100), vector2d<f32>(.5, .5), 90.f/DEG_RAD, vector2d<f32>(addedDim.Width/2,addedDim.Height/2));
	ff->fillMeshBuffer(testBuffer, addedString.c_str(), 4, false, SColor(255,0,0,255), .5f, &T);
	
	//testBuffer.getMaterial().setFlag(EMF_WIREFRAME, true);
	//testBuffer.getMaterial().MaterialType = EMT_SOLID;
	testBuffer.getMaterial().MaterialType = fmgr->getSDFFWithShadowsMaterialType();
	//testBuffer.getMaterial().MaterialType = fmgr->getSDFFMaterialType();
	
	FlexibleFontManager::Shadow testShadow{vector2df(0.0005,0.0005), 0.1, SColor(255,0,0,0)};
	ff->setDefaultShadow(testShadow);
	//chineseFont->setDefaultShadow(testShadow);
	
	IGUIEditBox* editBox = env->addEditBox(L"Test-Edit-Box", rect<s32>(1220,10,1600,500), true);
	editBox->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	editBox->setMultiLine(true);
	//editBox->setCursorChar(L'I');
	
	Drawer2D* drawer = new Drawer2D(device);
	
	ITexture* tex = driver->getTexture("../../Irrlicht/media/rsptnback.jpg");
	
	//TODO: simultaneous tests for fatness, italic, scale, border
	while(device->run()){
		if(device->isWindowActive()){
			driver->beginScene(true, true, SColor(255,240,240,240));//SColor(0,200,200,200));
			
			double t = getSecs();
			
			rect<s32> textRect(600,100,1200,700);
			ff->drawFontMeshBuffer(testBuffer, textRect, 1.5f, 0.5f, 0.05f, SColor(255,0,0,0), &testShadow, vector2d<irr::f32>(300.f,300.f), vector2d<irr::f32>(1.5f+sinf(t),1.5f+sinf(t+3.14)), t*10.0, vector2d<irr::f32>(testDim.Width/2, testDim.Height/2));
			ff->drawFontMeshBuffer(testBuffer, textRect, 1.5f, 0.5f, 0.05f, SColor(255,0,0,0), &testShadow);
			driver->draw2DRectangleOutline(textRect, SColor(255,0,0,0));
			driver->draw2DRectangleOutline(rect<s32>(899,399,901,401), SColor(255,0,0,0));
			driver->draw2DRectangleOutline(rect<s32>(600,100,600+testDim.Width,100+testDim.Height), SColor(255,0,0,0));
			
			rect<s32> textRect2(100, 550, 500, 650);
			ff->setDefaultScale(vector2df(1.f,1.f));
			ff->setDefaultFatness(0.4+0.2*sin(t));
			ff->setDefaultMaterialType(fmgr->getSDFFMaterialType());
			ff->draw(L"With methods from\ninterface", textRect2, SColor(255,0,0,0), true, true, &textRect2);
			ff->setDefaultFatness();
			//ff->setDefaultMaterialType(fmgr->getSDFFWithShadowsMaterialType());
			driver->draw2DRectangleOutline(textRect2, SColor(255,0,0,0));
			
			rect<s32> textRect3(100, 700, 500, 750);
			ff->setDefaultItalicGradient(0.3+0.3*sin(t));
			ff->setDefaultBorderSize(0.075f);
			ff->setDefaultBorderColor(SColor(255, 128+127*sin(t), 128+127*sin(2*t), 128+127*sin(3*t)));
			ff->draw(L"Test_\nbla\nblupp", textRect3, SColor(255,0,0,0), true, true, &textRect3);
			ff->setDefaultBorderSize();
			ff->setDefaultItalicGradient();
			driver->draw2DRectangleOutline(textRect3, SColor(255,0,0,0));
			
			//ff->setDefaultItalicGradient(0.5+0.5*sin(t));
			f32 scale = 2.5+2.0*sin(t);
			ff->setDefaultScale(vector2df(scale,scale));
			//ff->setDefaultFatness(0.4+0.2*sin(t));
			//ff->setDefaultSmoothingFactor(1.0+sin(t));
			
			bitmapFont->setDefaultScale(vector2df(scale,scale));
			rect<s32> textRect4(1220,550,1600,800);
			bitmapFont->draw(L"Test-Bitmap-Font", textRect4, SColor(255,0,0,0), true, true, &textRect4);
			driver->draw2DRectangleOutline(textRect4, SColor(255,0,0,0));
			
			chineseFont->setDefaultScale(vector2df(scale,scale));
			rect<s32> textRect5(600,750,1200,900);
			chineseFont->draw(L"谁读这个是愚蠢的", textRect5, SColor(255, 128+127*sin(2*t), 128+127*sin(t), 128+127*sin(3*t)), true, true, &textRect5);
			driver->draw2DRectangleOutline(textRect5, SColor(255,0,0,0));
			
			rect<s32> viewPort(100,100,500,500);
			driver->setViewPort(viewPort);
			drawer->setAutoResetTransformEnabled(false);
			drawer->draw(tex, rect<irr::s32>(10,10,100,100));
			drawer->draw(tex, vector2d<s32>(200+100*sin(t), 200+100*cos(t)), dimension2d<s32>(75,75), t*10.0, vector2d<f32>(.5f,.5f));
			drawer->draw(tex, rect<irr::s32>(325,325,400,400));
			drawer->draw(tex, vector2d<s32>(200, 200), dimension2d<s32>(75,75), t*10.0, vector2d<f32>(.5f,.5f));
			drawer->reset2DTransforms();
			drawer->setAutoResetTransformEnabled(true);
			driver->setViewPort(winRect);
			driver->draw2DRectangleOutline(viewPort, SColor(255,0,0,0));
			driver->draw2DRectangleOutline(rect<s32>(299,299,301,301), SColor(255,255,255,255));
			
			smgr->drawAll();
			env->drawAll();
			driver->endScene();
		}
	}
	
	delete drawer;
	
	fmgr->removeFont(chineseFont);//just for test
	fmgr->drop();
	
	device->drop();
	
	return 0;
}
