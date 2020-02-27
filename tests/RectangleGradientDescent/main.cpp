#include <timing.h>
#include <utilities.h>
#include <mathUtils.h>
#include <RectangleGradientDescent.h>

#include <irrlicht.h>

#include <list>
#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

#define USE_CONCURRENT
#define AVOID_PIVOT_RECTANGLES false

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
	
	device->setWindowCaption(L"Manual Tests for Rectangle Gradient Descent");
	
	const int maxX = driver->getScreenSize().Width;
	const int maxY = driver->getScreenSize().Height;
	
	std::list<RectangleGradientDescent::RectanglePair*> rectangles;
	
	#ifdef USE_CONCURRENT
	ConcurrentRectangleGradientDescent concDesc(rect<double>(0,0,maxX,maxY));
	RectangleGradientDescent& descent = *(concDesc.getRectangleGradientDescent());
	#else
	RectangleGradientDescent descent(rect<double>(0,0,maxX,maxY));
	#endif
	
	//srand(0);//to get the same values each test
	srand(time(NULL));
	
	const int maxRectWidth = maxX/6;
	const int minRectWidth = maxX/10;
	const int maxRectHeight = maxY/6;
	const int minRectHeight = maxY/10;
	//const int minDistance = maxY/8;
	//const int maxDistance = maxY/5;
	
	const int maxButtonSize = sqrt(maxX*maxX+maxY*maxY)*0.05;
	const int minButtonSize = maxButtonSize/2;
	
	for(int i=0; i<20; i++){//20
		rectangles.push_back(descent.addRectangle(RectangleGradientDescent::Rectangle{	vector2d<double>(rand()%maxX, rand()%maxY),
																													(double)(minRectWidth+rand()%(maxRectWidth-minRectWidth)),
																													(double)(minRectHeight+rand()%(maxRectHeight-minRectHeight)),
																													(double)((rand()%360)/DEG_RAD),
																													(double)(minButtonSize+rand()%(maxButtonSize-minButtonSize)),
																													(double)(minButtonSize+rand()%(maxButtonSize-minButtonSize)),
																													//(double)(minDistance+rand()%(maxDistance-minDistance)),
																													&descent.getSpaceLimit()}));//NULL}));//
	}
	
	#ifdef USE_CONCURRENT
	concDesc.start(400, 1.0/DEG_RAD, 0.1, AVOID_PIVOT_RECTANGLES);
	#else
	int counter = 0;
	#endif
	
	while(device->run()){
		
		driver->beginScene(true, true, SColor(255,240,240,240));
		
		#ifdef USE_CONCURRENT
		if(!concDesc.isRunning()){
		#else
		counter++;
		descent.optimize(1, 1.0/DEG_RAD, 0.1, AVOID_PIVOT_RECTANGLES);
		std::cout << "#steps: " << counter << std::endl;
		#endif
			for(auto it = rectangles.begin(); it != rectangles.end(); ++it){
				RectangleGradientDescent::Rectangle& r = (*it)->first;
				rect<s32> pivotRect(r.pivot.X-0.5*r.rectWidth, r.pivot.Y-0.5*r.rectHeight, r.pivot.X+0.5*r.rectWidth, r.pivot.Y+0.5*r.rectHeight);
				driver->draw2DRectangle(SColor(255,128,128,128), pivotRect);
			}

			for(auto it = rectangles.begin(); it != rectangles.end(); ++it){
				irr::core::rect<double>& dr = (*it)->second;
				rect<s32> r(dr.UpperLeftCorner.X, dr.UpperLeftCorner.Y, dr.LowerRightCorner.X, dr.LowerRightCorner.Y);
				driver->draw2DRectangleOutline(r, SColor(255,0,0,0));
				vector2d<double>& pivot = (*it)->first.pivot;
				driver->draw2DLine(r.getCenter(), vector2d<s32>(pivot.X, pivot.Y), SColor(255,0,0,0));
			}
		
		#ifdef USE_CONCURRENT
		}else{
			std::cout << "waiting: " << getSecs() << std::endl;
			device->getGUIEnvironment()->getSkin()->getFont()->draw(L"Waiting for rectangles", rect<s32>(0,0,maxX,maxY), SColor(255,0,0,0), true, true);
		}
		#endif
		
		driver->endScene();
		
		delay(1);
		
	}
	
	device->drop();
	
	return 0;
}
