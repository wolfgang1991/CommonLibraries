#include <IExtendableSkin.h>
#include <ScrollBarSkinExtension.h>
#include <AggregatableGUIElementAdapter.h>
#include <AggregateGUIElement.h>
#include <timing.h>
#include <utilities.h>
#include <StringHelpers.h>
#include <ScrollBar.h>
#include <Drawer2D.h>
#include <BeautifulGUIImage.h>
#include <utilities.h>
#include <AggregateSkinExtension.h>
#include <DraggableGUIElement.h>
#include <DragPlaceGUIElement.h>
#include <NotificationBox.h>
#include <mathUtils.h>

#include <irrlicht.h>

#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

void printElementTree(irr::gui::IGUIElement* root, int tabCount = 0){
	 const core::list<IGUIElement*>& children = root->getChildren();
	 for(int i=0; i<tabCount; i++){
	 	std::cout << '\t';
	 }
	 std::cout << root->getName() << std::endl;
	 tabCount++;
	 for(auto it = children.begin(); it != children.end(); ++it){
	 	printElementTree(*it, tabCount);
	 }
}

class AppSkin : public IExtendableSkin {

	public:
	
	enum SkinIDs{
		DEFAULT_AGGREGATABLE,
		REGULAR_SCROLLBAR,
		REGULAR_AGGREGATION,
		NO_HIGHLIGHT_AGGREGATION,
		INVISIBLE_AGGREGATION,
		ID_COUNT
	};

	AppSkin(irr::IrrlichtDevice* device):
		IExtendableSkin(device->getGUIEnvironment()->createSkin(gui::EGST_WINDOWS_CLASSIC), device){
		registerExtension(new ScrollBarSkinExtension(this, {SColor(255,255,255,255), SColor(255,196,198,201)}, .1f, .3f), REGULAR_SCROLLBAR);
		registerExtension(new AggregateSkinExtension(this, true, true), REGULAR_AGGREGATION);
		registerExtension(new AggregateSkinExtension(this, false, true), NO_HIGHLIGHT_AGGREGATION);
		registerExtension(new AggregateSkinExtension(this, false, false), INVISIBLE_AGGREGATION);
		registerExtension(new DefaultAggregatableSkin(this, true), DEFAULT_AGGREGATABLE);
	}
		
	~AppSkin(){
		parent->drop();//drop required here since parent is created in this constructor
	}
		
};

int main(int argc, char *argv[]){
	IrrlichtDevice* nulldevice = createDevice(EDT_NULL);
	core::dimension2d<u32> deskres = nulldevice->getVideoModeList()->getDesktopResolution();
	nulldevice->drop();
	
	SIrrlichtCreationParameters param;
	param.Bits = 32;
   param.DriverType = EDT_OPENGL;
	bool portrait = false;
	float ratio = 16.f/9.f;
	rect<irr::s32> winRect(0,0,9*deskres.Width/10,9*deskres.Height/10);
	winRect = makeXic(winRect, portrait?(1.f/ratio):ratio);
	param.WindowSize = winRect.getSize();
	
	IrrlichtDevice* device = createDeviceEx(param);
	IVideoDriver* driver = device->getVideoDriver();
	IGUIEnvironment* env = device->getGUIEnvironment();
	ISceneManager* smgr = device->getSceneManager();
	
	Drawer2D* drawer = new Drawer2D(device);
	
	device->setWindowCaption(L"Manual Tests for GUI Stuff");
	
	AppSkin* skin = new AppSkin(device);
	assert(isExtendableSkin(skin));
	env->setSkin(skin);
	skin->drop();
	
	rect<s32> testArea(20, 20, winRect.getWidth()/4, winRect.getHeight()/4);
	bool scrollable = true;
	bool horizontal = true;
	new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, horizontal, scrollable, 	{
		new AggregatableGUIElementAdapter(env, .3f, 10.f/1.f, true, env->addStaticText(L"Test. Blabla.", rect<s32>(0,0,0,0), true), true, AppSkin::DEFAULT_AGGREGATABLE),
		new AggregatableGUIElementAdapter(env, .5f, 1.f/1.f, true, env->addEditBox(L"Edit me", rect<s32>(0,0,0,0)), true, AppSkin::DEFAULT_AGGREGATABLE),
		new EmptyGUIElement(env, .5f, 1.f/1.f, true, true, AppSkin::DEFAULT_AGGREGATABLE),
		new AggregatableGUIElementAdapter(env, .9f, 2.f/1.f, true, env->addComboBox(rect<s32>(0,0,0,0)), true, AppSkin::DEFAULT_AGGREGATABLE)
	}, {}, false, AppSkin::REGULAR_AGGREGATION, NULL, NULL, testArea);
	
	
	rect<s32> testArea2(20, winRect.getHeight()/4+20, winRect.getWidth()/4, winRect.getHeight()/2);
	scrollable = false;
	horizontal = false;
	new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, horizontal, scrollable, 	{
		new AggregatableGUIElementAdapter(env, .3f, 10.f/1.f, true, env->addStaticText(L"Test. Blabla.", rect<s32>(0,0,0,0), true), true, AppSkin::DEFAULT_AGGREGATABLE),
		new AggregatableGUIElementAdapter(env, .5f, 1.f/1.f, true, env->addEditBox(L"Edit me", rect<s32>(0,0,0,0)), false, AppSkin::DEFAULT_AGGREGATABLE),
		new EmptyGUIElement(env, .5f, 1.f/1.f, true, false, AppSkin::DEFAULT_AGGREGATABLE),
		new AggregatableGUIElementAdapter(env, .9f, 2.f/1.f, true, env->addComboBox(rect<s32>(0,0,0,0)), false, AppSkin::DEFAULT_AGGREGATABLE)
	}, {}, false, AppSkin::REGULAR_AGGREGATION, NULL, NULL, testArea2);
	
																				
	rect<s32> testArea3(20, winRect.getHeight()/2+20, winRect.getWidth()/4, winRect.getHeight());
	scrollable = true;
	horizontal = false;
	AggregateGUIElement* a3 = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, horizontal, scrollable, {}, {}, false, AppSkin::REGULAR_AGGREGATION, NULL, NULL, testArea3);
	
	for(int i=0; i<10; i++){
		AggregateGUIElement* row = new AggregateGUIElement(env, .3, 1.f, .3, 1.f, false, true, true, 	{
			new AggregatableGUIElementAdapter(env, .6f, 10.f/1.f, true, env->addStaticText(std::wstring(L"Label ").append(convertToWString(i)).c_str(), rect<s32>(0,0,0,0), true), false, AppSkin::DEFAULT_AGGREGATABLE),
			new AggregatableGUIElementAdapter(env, .5f, 2.f/1.f, true, env->addEditBox(L"Edit me", rect<s32>(0,0,0,0)), false, AppSkin::DEFAULT_AGGREGATABLE)
		}, {}, true, AppSkin::REGULAR_AGGREGATION);
		a3->addSubElement(row);
		ScrollBar* rowScroll = new ScrollBar(env, .05f, true, AppSkin::REGULAR_SCROLLBAR);
		rowScroll->linkToScrollable(row);
		a3->addSubElement(rowScroll);
	}
	
	rect<s32> s1Rect(winRect.getWidth()/4+20, winRect.getHeight()/2+20, 5*winRect.getWidth()/16, winRect.getHeight());
	ScrollBar* s1 = new ScrollBar(env, .1f, false, AppSkin::REGULAR_SCROLLBAR, NULL, NULL, s1Rect);
	s1->linkToScrollable(a3);
	
	//a3->setMultiSelectable(true);
	
	rect<s32> testArea4(winRect.getWidth()/4+20, 20, 3*winRect.getWidth()/4, winRect.getHeight()/2);
	scrollable = true;
	horizontal = false;
	AggregateGUIElement* a4 = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, horizontal, scrollable, {}, {}, false, AppSkin::REGULAR_AGGREGATION, NULL, NULL, testArea4);
	for(int i=0; i<20; i++){
		AggregateGUIElement* row = new AggregateGUIElement(env, .15, 1.f, .3, 1.f, false, true, false, {
			new EmptyGUIElement(env, .025f, 1.f/1.f, false, false, AppSkin::DEFAULT_AGGREGATABLE),
			new AggregateGUIElement(env, .1, 1.f, .1, 1.f, false, false, false, {
				new EmptyGUIElement(env, .1f, 1.f/1.f, false, false, AppSkin::DEFAULT_AGGREGATABLE),
				new BeautifulGUIImage(drawer, driver->getTexture("media/boxGradient.png"), env, .8f, false, AppSkin::DEFAULT_AGGREGATABLE, SColor(255,i%10*25,i%6*42,i%3*85)),
				new EmptyGUIElement(env, .1f, 1.f/1.f, false, false, AppSkin::DEFAULT_AGGREGATABLE)
			}, {}, false, AppSkin::INVISIBLE_AGGREGATION),
			addAggregatableStaticText(env, L"xy%", EGUIA_CENTER, EGUIA_CENTER, .2f),
			new BeautifulGUIImage(drawer, driver->getTexture(i%2==0?"media/bin.png":"media/rename.png"), env, .3f, true, AppSkin::DEFAULT_AGGREGATABLE),
			addAggregatableStaticText(env, L"Txy", EGUIA_CENTER, EGUIA_CENTER, .15f),
			addAggregatableStaticText(env, L"TOOLTYPE", EGUIA_CENTER, EGUIA_CENTER, .4f),
			addAggregatableStaticText(env, L"BASIC INFORMATION", EGUIA_CENTER, EGUIA_CENTER, .5f),
			new AggregatableGUIElementAdapter(env, .2f, 2.f/1.f, true, env->addButton(rect<s32>(0,0,0,0), NULL, -1, L"Delete"), false, AppSkin::DEFAULT_AGGREGATABLE)
		}, {
			addAggregatableStaticText(env, L"Test", EGUIA_CENTER, EGUIA_CENTER, .2f),
		}, true, AppSkin::NO_HIGHLIGHT_AGGREGATION);
		a4->addSubElement(row);
	}
	
	rect<s32> s2Rect(3*winRect.getWidth()/4+20, 20, 13*winRect.getWidth()/16, winRect.getHeight()/2);
	ScrollBar* s2 = new ScrollBar(env, .1f, false, AppSkin::REGULAR_SCROLLBAR, NULL, NULL, s2Rect);
	s2->linkToScrollable(a4);
	
	//Targets where tools can be dropped
	rect<s32> testArea6(5*winRect.getWidth()/16+20, 3*winRect.getHeight()/4+20, 3*winRect.getWidth()/4, winRect.getHeight());
	scrollable = false;
	horizontal = true;
	AggregateGUIElement* a6 = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, horizontal, scrollable, {}, {}, false, AppSkin::REGULAR_AGGREGATION, NULL, NULL, testArea6);
	std::list<DragPlaceGUIElement*> targets;
	for(int i=0; i<5; i++){
		DragPlaceGUIElement* dp = new DragPlaceGUIElement(env, .3f, 1.f, false, AppSkin::DEFAULT_AGGREGATABLE, new BeautifulGUIImage(drawer, driver->getTexture("media/reminder.png"), env, 1.f, true, AppSkin::DEFAULT_AGGREGATABLE), NULL);
		a6->addSubElement(dp);
		targets.push_back(dp);
	}
	
	//Tools
	rect<s32> testArea5(5*winRect.getWidth()/16+20, winRect.getHeight()/2+20, 3*winRect.getWidth()/4, 3*winRect.getHeight()/4);
	scrollable = true;
	horizontal = true;
	AggregateGUIElement* a5 = new AggregateGUIElement(env, 1.f, 1.f, 1.f, 1.f, false, horizontal, scrollable, {}, {}, false, AppSkin::REGULAR_AGGREGATION, NULL, NULL, testArea5);
	vector2d<u32> verticalScrollThreshold(~(u32)0, testArea5.getHeight()/8);
	for(int i=0; i<20; i++){
		DragPlaceGUIElement* dp = new DragPlaceGUIElement(env, .3f, 1.f, false, AppSkin::DEFAULT_AGGREGATABLE, NULL, NULL);
		new DraggableGUIElement(env, -1, dimension2d<u32>(.1*winRect.getWidth(), .2*winRect.getHeight()), dp, targets, verticalScrollThreshold,
			new AggregateGUIElement(env, .6, 1.f, .3, 1.f, false, true, false, {
				new BeautifulGUIImage(drawer, driver->getTexture("media/bin.png"), env, .7, true, AppSkin::DEFAULT_AGGREGATABLE),
				addAggregatableStaticText(env, std::wstring(L"Blablabla\nT").append(convertToWString(i)).c_str(), EGUIA_CENTER, EGUIA_CENTER, .3f)
			}, {}, true, AppSkin::NO_HIGHLIGHT_AGGREGATION),
			new BeautifulGUIImage(drawer, driver->getTexture("media/bin.png"), env, .7, true, AppSkin::DEFAULT_AGGREGATABLE),
			new AggregateGUIElement(env, .3, 1.f, .3, 1.f, false, false, false, {
				new BeautifulGUIImage(drawer, driver->getTexture("media/bin.png"), env, .7, true, AppSkin::DEFAULT_AGGREGATABLE),
				addAggregatableStaticText(env, std::wstring(L"T").append(convertToWString(i)).c_str(), EGUIA_CENTER, EGUIA_CENTER, .3f)
			}, {}, true, AppSkin::NO_HIGHLIGHT_AGGREGATION));
		a5->addSubElement(dp);
	}
	
	new NotificationBox(10000, device, L"Blablabla Blablabla Blablabla Blablabla Blablabla Blablabla Blablabla Blablabla Blablabla", .75, .75, L"Ok", NULL, -1, -1, false);
	
	while(device->run()){
		if(device->isWindowActive()){
			driver->beginScene(true, true, SColor(255,240,240,240));//SColor(0,200,200,200));
			smgr->drawAll();
			env->drawAll();
//			driver->draw2DRectangleOutline(testArea, SColor(255,0,0,0));
//			driver->draw2DRectangleOutline(testArea2, SColor(255,0,0,0));
//			driver->draw2DRectangleOutline(testArea3, SColor(255,0,0,0));
//			driver->draw2DRectangleOutline(s1Rect, SColor(255,0,0,0));
			driver->endScene();
			//printElementTree(a6);
		}
		delay(1);
	}
	
	delete drawer;
	
	device->drop();
	
	return 0;
}
