#include <Drawer2D.h>
#include <timing.h>

#include <irrlicht.h>

#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

static void drawCheckPoly(IVideoDriver* driver, irr::core::array< irr::core::vector2d<irr::f32> >& p, bool closed, SColor color = SColor(255,255,0,0)){
	if(p.size()<=1){return;}
	for(uint32_t i=0; i<p.size()-(closed?0:1); i++){
		vector2d<irr::f32>& thisP = p[i];
		vector2d<irr::f32>& nextP = p[(i+1)%p.size()];
		driver->draw2DLine(vector2d<s32>(thisP.X, thisP.Y), vector2d<s32>(nextP.X, nextP.Y), color);
	}
}

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
	
	device->setWindowCaption(L"Manual Tests for Polygon Stuff");
	
	ITexture* dashedStrong = driver->getTexture("dashedStrong.png");
	
	Drawer2D* drawer = new Drawer2D(device);
	//drawer->getMaterial().setFlag(EMF_WIREFRAME, true);
	E_MATERIAL_TYPE overrideMatType = EMT_SOLID;
	drawer->setOverridePolygonMaterialType(&overrideMatType);
	//drawer->setDefaultPolygonAAValue(.25f);
	
	irr::core::array< vector2d<f32> > simpleRect;
	simpleRect.push_back(vector2d<f32>(100,100));
	simpleRect.push_back(vector2d<f32>(300,100));
	simpleRect.push_back(vector2d<f32>(300,300));
	simpleRect.push_back(vector2d<f32>(100,300));
	f32 simpleRectHandle = .5f;
	bool simpleRectClosed = true;
	
	irr::core::array< vector2d<f32> > simpleLine;
	simpleLine.push_back(vector2d<f32>(100,400));
	simpleLine.push_back(vector2d<f32>(200,400));
	simpleLine.push_back(vector2d<f32>(300,500));
	f32 simpleLineHandle = 1.f;
	bool simpleLineClosed = false;
	
	irr::core::array< vector2d<f32> > sharpCornerLine;
	sharpCornerLine.push_back(vector2d<f32>(400,100));
	sharpCornerLine.push_back(vector2d<f32>(500,300));
	sharpCornerLine.push_back(vector2d<f32>(500,100));
	sharpCornerLine.push_back(vector2d<f32>(600,300));
	f32 sharpCornerLineHandle = .5f;
	bool sharpCornerLineClosed = false;
	
	irr::core::array< vector2d<f32> > sharpCornerLine2;
	sharpCornerLine2.push_back(vector2d<f32>(800,100));
	sharpCornerLine2.push_back(vector2d<f32>(900,300));
	sharpCornerLine2.push_back(vector2d<f32>(900,100));
	sharpCornerLine2.push_back(vector2d<f32>(1000,300));
	f32 sharpCornerLine2Handle = .5f;
	bool sharpCornerLine2Closed = false;
	
	const double deg2rad = 0.0174532925199433;
	
	irr::core::array< vector2d<f32> > spiral;
	f32 cx = 1000, cy = 600;
	for(uint32_t i=0; i<500; i++){
		uint32_t r = 20+(200*i)/500;
		uint32_t phi = (4*i)%360;
		spiral.push_back(vector2d<f32>(cx+r*sin(phi*deg2rad),cy+r*cos(phi*deg2rad)));
	}
	f32 spiralHandle = 0.f;
	bool spiralClosed = false;
	
	SColor polyColor(255,0,0,0);
	int polyThickness = 20;
	
	bool weldedVertices = false;
	
	while(device->run()){
		if(device->isWindowActive()){
			driver->beginScene(true, true, SColor(255,240,240,240));//SColor(0,200,200,200));

			f32 lineHandle = fabs(sin(getSecs()));//0.f;//

			drawer->getMaterial().setFlag(EMF_WIREFRAME, false);
			drawer->setOverridePolygonMaterialType(NULL);
			
			drawer->drawPolygon(sharpCornerLine2, SColor(255,0,128,255), polyThickness, 0.1f, dashedStrong, sharpCornerLine2Closed, lineHandle, weldedVertices);
			drawCheckPoly(driver, sharpCornerLine2, sharpCornerLine2Closed);

			drawer->drawPolygon(spiral, SColor(255,255,255,0), polyThickness, 0.025f, dashedStrong, spiralClosed, lineHandle, weldedVertices);
			drawCheckPoly(driver, spiral, spiralClosed);
			
			drawer->getMaterial().setFlag(EMF_WIREFRAME, true);
			drawer->setOverridePolygonMaterialType(&overrideMatType);
			
			drawer->drawPolygon(simpleRect, polyColor, polyThickness, 0.01f, NULL, simpleRectClosed, lineHandle, weldedVertices);
			drawCheckPoly(driver, simpleRect, simpleRectClosed);
			
			drawer->drawPolygon(simpleLine, polyColor, polyThickness, 0.01f, NULL, simpleLineClosed, lineHandle, weldedVertices);
			drawCheckPoly(driver, simpleLine, simpleLineClosed);
			
			drawer->drawPolygon(sharpCornerLine, polyColor, polyThickness, 0.01f, NULL, sharpCornerLineClosed, lineHandle, weldedVertices);
			drawCheckPoly(driver, sharpCornerLine, sharpCornerLineClosed);
			
			drawer->drawPolygon(spiral, polyColor, polyThickness, 0.01f, NULL, spiralClosed, lineHandle, weldedVertices);
			drawCheckPoly(driver, spiral, spiralClosed);
			
			driver->endScene();
		}
	}

	delete drawer;
	
	device->drop();
	
	return 0;
}
