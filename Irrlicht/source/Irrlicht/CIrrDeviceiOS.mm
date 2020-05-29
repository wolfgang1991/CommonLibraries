// Copyright (C) 2002-2008 Nikolaus Gebhardt
// Copyright (C) 2008 Redshift Software, Inc.
// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#import "CIrrDeviceiOS.h"

#ifdef _IRR_COMPILE_WITH_IOS_DEVICE_

#include "iOSViewController.h"//changed
#include "IFileSystem.h"
#include "CTimer.h"
#include "CEAGLManager.h"

#import <UIKit/UIKit.h>
//changed: #import <CoreMotion/CoreMotion.h>

/* Important information */

// The application state events and following methods: IrrlichtDevice::isWindowActive, IrrlichtDevice::isWindowFocused
// and IrrlichtDevice::isWindowMinimized works out of box only if you'll use built-in CIrrDelegateiOS,
// so _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_ must be enabled in this case. If you need a custom UIApplicationDelegate you must
// handle all application events yourself.

#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_

namespace irr
{
	class CIrrDeviceiOS;
}

//changed
#define KEYMAPSIZE 256

@implementation ViewController
{
    bool autorotate;
    bool statusBarHidden;
    bool shallKeyboardBeVisible;
    irr::EKEY_CODE keyMap[KEYMAPSIZE];
    irr::SEvent keyEvent;
    irr::IrrlichtDevice* device;
}

- (void)setIrrlichtDevice:(irr::IrrlichtDevice*)device
{
    self->device = device;
}

- (void)setShouldAutoRotate:(bool)autoRotate
{
    self->autorotate = autoRotate;
}

- (void)setPreferStatusBarHidden:(bool)statusBarHidden
{
    self->statusBarHidden = statusBarHidden;
    [self setNeedsStatusBarAppearanceUpdate];
}

- (id)init
{
    self = [super init];
    autorotate = false;//autorotation requires recalculation of screen coordinated inside app (I guess it is usually not wanted)
    statusBarHidden = true;
    shallKeyboardBeVisible = false;
    for(int i=0; i<KEYMAPSIZE; i++){
        keyMap[i] = irr::KEY_UNKNOWN;
    }
    keyMap[(int)'\t'] = irr::KEY_TAB;
    keyMap[(int)'\r'] = irr::KEY_RETURN;//carriage return (just in case)
    keyMap[(int)'\n'] = irr::KEY_RETURN;
    keyMap[(int)' '] = irr::KEY_SPACE;
    for(int i=0; i<10; i++){
        keyMap[(int)'0'+i] = (irr::EKEY_CODE)((int)irr::KEY_KEY_0+i);
    }
    for(int i=0; i<26; i++){
        irr::EKEY_CODE code = (irr::EKEY_CODE)((int)irr::KEY_KEY_A+i);
        keyMap[(int)'A'+i] = code;
        keyMap[(int)'a'+i] = code;
    }
    keyMap[(int)'+'] = irr::KEY_PLUS;
    keyMap[(int)','] = irr::KEY_COMMA;
    keyMap[(int)'-'] = irr::KEY_MINUS;
    keyMap[(int)'.'] = irr::KEY_PERIOD;
    keyMap[(int)'*'] = irr::KEY_MULTIPLY;
    keyMap[(int)'/'] = irr::KEY_DIVIDE;
    keyEvent.EventType = irr::EET_KEY_INPUT_EVENT;
    keyEvent.KeyInput.Char = L'0';
    keyEvent.KeyInput.Control = false;
    keyEvent.KeyInput.Key = irr::KEY_KEY_0;
    keyEvent.KeyInput.PressedDown = false;
    keyEvent.KeyInput.Shift = false;
    keyEvent.KeyInput.SystemKeyCode = 0;
    device = NULL;
    self.autocorrectionType = UITextAutocorrectionTypeNo;//disable auto correction
    return self;
}

- (BOOL)shouldAutorotate
{
    return autorotate;
}

- (BOOL)prefersStatusBarHidden {
    return statusBarHidden;
}

- (void)setKeyboardVisibility:(bool)visibility
{
    //printf("setKeyboardVisibility: %i\n", (int)visibility);
    shallKeyboardBeVisible = visibility;
    if(visibility)
    {
        [self becomeFirstResponder];
    }else
    {
        [self resignFirstResponder];
    }
}


-(void) deleteBackward: (id)sender
{
    [self deleteBackward];
    return;
}

-(BOOL) hasText
{
    return NO;//YES;
}

-(void) insertText:(NSString *)text
{
    const char* mbtext = [text cStringUsingEncoding:NSUTF32StringEncoding];//wchar_t is UTF32 on MacOS and iOS
    if(mbtext!=NULL){
        const wchar_t* ctext = reinterpret_cast<const wchar_t*>(mbtext);
        keyEvent.KeyInput.SystemKeyCode = reinterpret_cast<const irr::u32&>(ctext[0]);
        //wprintf(L"ctext: %ls mbtext: %s (%i)\n", ctext, mbtext, keyEvent.KeyInput.SystemKeyCode);
        keyEvent.KeyInput.Char = ctext[0];
        keyEvent.KeyInput.Key = keyMap[(unsigned char)(mbtext[0])];//in 0-255, because of unsigned char
        if(device){
            keyEvent.KeyInput.PressedDown = true;
            device->postEventFromUser(keyEvent);
            keyEvent.KeyInput.PressedDown = false;
            device->postEventFromUser(keyEvent);
        }
    }
}

- (void)deleteBackward {
    keyEvent.KeyInput.Char = 0x08;//same char as in other operating systems
    keyEvent.KeyInput.SystemKeyCode = 0x08;
    keyEvent.KeyInput.Key = irr::KEY_BACK;
    if(device){
        keyEvent.KeyInput.PressedDown = true;
        device->postEventFromUser(keyEvent);
        keyEvent.KeyInput.PressedDown = false;
        device->postEventFromUser(keyEvent);
    }
}


-(BOOL) canBecomeFirstResponder
{
    return shallKeyboardBeVisible;//YES;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self setKeyboardVisibility:false];
}

@end
//changed end

/* CIrrDelegateiOS */

@interface CIrrDelegateiOS : NSObject<UIApplicationDelegate>

- (void)setDevice:(irr::CIrrDeviceiOS*)device;
- (bool)isActive;
- (bool)hasFocus;

@property (strong, nonatomic) UIWindow* window;

@end

@implementation CIrrDelegateiOS
{
	irr::CIrrDeviceiOS* Device;
	bool Active;
	bool Focus;
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options
{
	Device = nil;
	Active = true;
	Focus = false;
	
	[self performSelectorOnMainThread:@selector(runIrrlicht) withObject:nil waitUntilDone:NO];
	
	return YES;
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	if (Device != nil)
	{
		printf("applicationWillTerminate\n");
		irr::SEvent ev;
		ev.EventType = irr::EET_APPLICATION_EVENT;
		ev.ApplicationEvent.EventType = irr::EAET_WILL_TERMINATE;

		Device->postEventFromUser(ev);
		
		Device->closeDevice();
	}
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
	if (Device != nil)
	{
		printf("applicationDidReceiveMemoryWarning\n");
		irr::SEvent ev;
		ev.EventType = irr::EET_APPLICATION_EVENT;
		ev.ApplicationEvent.EventType = irr::EAET_MEMORY_WARNING;
		
		Device->postEventFromUser(ev);
	}
}

- (void)applicationWillResignActive:(UIApplication*)application
{
	if (Device != nil)
	{
		printf("applicationWillResignActive\n");
		irr::SEvent ev;
		ev.EventType = irr::EET_APPLICATION_EVENT;
		ev.ApplicationEvent.EventType = irr::EAET_WILL_PAUSE;
		
		Device->postEventFromUser(ev);
	}
	
	Focus = false;
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
	if (Device != nil)
	{
		printf("applicationDidEnterBackground\n");
		irr::SEvent ev;
		ev.EventType = irr::EET_APPLICATION_EVENT;
		ev.ApplicationEvent.EventType = irr::EAET_DID_PAUSE;
		
		Device->postEventFromUser(ev);
	}
	
	Active = false;
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
	if (Device != nil)
	{
		printf("applicationWillEnterForeground\n");
		irr::SEvent ev;
		ev.EventType = irr::EET_APPLICATION_EVENT;
		ev.ApplicationEvent.EventType = irr::EAET_WILL_RESUME;
		
		Device->postEventFromUser(ev);
	}
	
	Active = true;
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
	if (Device != nil)
	{
		printf("applicationDidBecomeActive\n");
		irr::SEvent ev;
		ev.EventType = irr::EET_APPLICATION_EVENT;
		ev.ApplicationEvent.EventType = irr::EAET_DID_RESUME;
		
		Device->postEventFromUser(ev);
	}
	
	Focus = true;
}

- (void)runIrrlicht
{
	irrlicht_main();
}

- (void)setDevice:(irr::CIrrDeviceiOS*)device
{
	Device = device;
}

- (bool)isActive
{
	return Active;
}

- (bool)hasFocus
{
	return Focus;
}

@end

#endif

/* CIrrViewiOS */

@interface CIrrViewiOS : UIView

- (id)initWithFrame:(CGRect)frame forDevice:(irr::CIrrDeviceiOS*)device;

@end

@implementation CIrrViewiOS
{
    irr::CIrrDeviceiOS* Device;
    float Scale;
}

- (id)initWithFrame:(CGRect)frame forDevice:(irr::CIrrDeviceiOS*)device;
{
    self = [super initWithFrame:frame];
    
    if (self)
    {
        Device = device;
        Scale = ([self respondsToSelector:@selector(setContentScaleFactor:)]) ? [[UIScreen mainScreen] scale] : 1.f;
    }
    
    return self;
}

- (BOOL)isMultipleTouchEnabled
{
	return YES;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	irr::SEvent ev;
	ev.EventType = irr::EET_TOUCH_INPUT_EVENT;
	ev.TouchInput.Event = irr::ETIE_PRESSED_DOWN;
    
	for (UITouch* touch in touches)
	{
        ev.TouchInput.ID = Device->getTouchIndex((__bridge void*)touch);//(size_t)touch; //changed
		CGPoint touchPoint = [touch locationInView:self];
        
        ev.TouchInput.X = touchPoint.x*Scale;
        ev.TouchInput.Y = touchPoint.y*Scale;

        if(ev.TouchInput.ID==0){Device->touch2MouseEvent(ev.TouchInput);}//changed
        Device->postEventFromUser(ev);
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	irr::SEvent ev;
	ev.EventType = irr::EET_TOUCH_INPUT_EVENT;
	ev.TouchInput.Event = irr::ETIE_MOVED;
    
	for (UITouch* touch in touches)
	{
        ev.TouchInput.ID = Device->getTouchIndex((__bridge void*)touch);//(size_t)touch; //changed

		CGPoint touchPoint = [touch locationInView:self];
        
        ev.TouchInput.X = touchPoint.x*Scale;
        ev.TouchInput.Y = touchPoint.y*Scale;
        
        if(ev.TouchInput.ID==0){Device->touch2MouseEvent(ev.TouchInput);}//changed
        Device->postEventFromUser(ev);
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	irr::SEvent ev;
	ev.EventType = irr::EET_TOUCH_INPUT_EVENT;
	ev.TouchInput.Event = irr::ETIE_LEFT_UP;
    
	for (UITouch* touch in touches)
	{
        ev.TouchInput.ID = Device->removeTouch((__bridge void*)touch);//(size_t)touch;//changed
		CGPoint touchPoint = [touch locationInView:self];
        
        ev.TouchInput.X = touchPoint.x*Scale;
        ev.TouchInput.Y = touchPoint.y*Scale;
        
        if(ev.TouchInput.ID==0){Device->touch2MouseEvent(ev.TouchInput);}//changed
        Device->postEventFromUser(ev);
	}
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	irr::SEvent ev;
	ev.EventType = irr::EET_TOUCH_INPUT_EVENT;
	ev.TouchInput.Event = irr::ETIE_LEFT_UP;
    
	for (UITouch* touch in touches)
	{
        ev.TouchInput.ID = Device->getTouchIndex((__bridge void*)touch);//(size_t)touch; //changed
        
		CGPoint touchPoint = [touch locationInView:self];
        
        ev.TouchInput.X = touchPoint.x*Scale;
        ev.TouchInput.Y = touchPoint.y*Scale;
        
        if(ev.TouchInput.ID==0){Device->touch2MouseEvent(ev.TouchInput);}//changed
        Device->postEventFromUser(ev);
	}
}

@end

/* CIrrViewEAGLiOS */

@interface CIrrViewEAGLiOS : CIrrViewiOS

@end

@implementation CIrrViewEAGLiOS

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

@end

namespace irr
{
	namespace video
	{
		IVideoDriver* createOGLES1Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);
		
		IVideoDriver* createOGLES2Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);
	}
	
    struct SIrrDeviceiOSDataStorage
    {
        SIrrDeviceiOSDataStorage() : Window(0), ViewController(0), View(0)//changed: , MotionManager(0), ReferenceAttitude(0)
        {
            //changed: MotionManager = [[CMMotionManager alloc] init];
        }
        
        UIWindow* Window;
        UIViewController* ViewController;
        CIrrViewiOS* View;
        //changed: CMMotionManager* MotionManager;
        //changed: CMAttitude* ReferenceAttitude;
    };
    
    CIrrDeviceiOS::CIrrDeviceiOS(const SIrrlichtCreationParameters& params) : CIrrDeviceStub(params), DataStorage(0), Close(false)
    {
#ifdef _DEBUG
        setDebugName("CIrrDeviceiOS");
#endif
		for(unsigned int i=0; i<touchCount; i++){touch2Index[i] = NULL;}//changed
#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
		CIrrDelegateiOS* delegate = [UIApplication sharedApplication].delegate;
		[delegate setDevice:this];
#endif
        
        DataStorage = new SIrrDeviceiOSDataStorage();

        FileSystem->changeWorkingDirectoryTo([[[NSBundle mainBundle] resourcePath] UTF8String]);

		createWindow();
        createViewAndDriver();
        
        if (!VideoDriver)
            return;
        
        CursorControl = new CCursorControl(this, VideoDriver->getScreenSize());//changed
        
        createGUIAndScene();
    }

    CIrrDeviceiOS::~CIrrDeviceiOS()
    {
        deactivateDeviceMotion();
        deactivateGyroscope();
        deactivateAccelerometer();
        
        delete static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);

#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
		CIrrDelegateiOS* delegate = [UIApplication sharedApplication].delegate;
		[delegate setDevice:nil];
#endif
    }

    bool CIrrDeviceiOS::run()
    {
		if (!Close)
		{
			const CFTimeInterval timeInSeconds = 0.000002;
			
			s32 result = 0;
			
			do
			{
				result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, timeInSeconds, TRUE);
			}
			while (result == kCFRunLoopRunHandledSource);
			
			os::Timer::tick();
			
			//! Update events
			/* changed:
			SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
			CMMotionManager* motionManager = dataStorage->MotionManager;
			
			//! Accelerometer
			if (motionManager.isAccelerometerActive)
			{
				irr::SEvent ev;
				ev.EventType = irr::EET_ACCELEROMETER_EVENT;
				ev.AccelerometerEvent.X = motionManager.accelerometerData.acceleration.x;
				ev.AccelerometerEvent.Y = motionManager.accelerometerData.acceleration.y;
				ev.AccelerometerEvent.Z = motionManager.accelerometerData.acceleration.z;
				
				postEventFromUser(ev);
			}
			
			//! Gyroscope
			if (motionManager.isGyroActive)
			{
				irr::SEvent ev;
				ev.EventType = irr::EET_GYROSCOPE_EVENT;
				ev.GyroscopeEvent.X = motionManager.gyroData.rotationRate.x;
				ev.GyroscopeEvent.Y = motionManager.gyroData.rotationRate.y;
				ev.GyroscopeEvent.Z = motionManager.gyroData.rotationRate.z;
				
				postEventFromUser(ev);
			}
			
			//! Device Motion
			if (motionManager.isDeviceMotionActive)
			{
				CMAttitude* currentAttitude = motionManager.deviceMotion.attitude;
				CMAttitude* referenceAttitude = dataStorage->ReferenceAttitude;
				
				if (referenceAttitude != nil)
					[currentAttitude multiplyByInverseOfAttitude: referenceAttitude];
				else
					referenceAttitude = motionManager.deviceMotion.attitude;
				
				irr::SEvent ev;
				ev.EventType = irr::EET_DEVICE_MOTION_EVENT;
				ev.AccelerometerEvent.X = currentAttitude.roll;
				ev.AccelerometerEvent.Y = currentAttitude.pitch;
				ev.AccelerometerEvent.Z = currentAttitude.yaw;
				
				postEventFromUser(ev);
			}
            */
		}

		return !Close;
    }

    void CIrrDeviceiOS::yield()
    {
        struct timespec ts = {0,0};
        nanosleep(&ts, NULL);
    }

    void CIrrDeviceiOS::sleep(u32 timeMs, bool pauseTimer=false)
    {
        bool wasStopped = Timer ? Timer->isStopped() : true;
		
        struct timespec ts;
        ts.tv_sec = (time_t) (timeMs / 1000);
        ts.tv_nsec = (long) (timeMs % 1000) * 1000000;
        
        if (pauseTimer && !wasStopped)
            Timer->stop();
        
        nanosleep(&ts, NULL);
        
        if (pauseTimer && !wasStopped)
            Timer->start();
    }

    void CIrrDeviceiOS::setWindowCaption(const wchar_t* text)
    {
    }
    
    bool CIrrDeviceiOS::isWindowActive() const
    {
#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
		CIrrDelegateiOS* delegate = [UIApplication sharedApplication].delegate;
		
		return [delegate isActive];
#else
		return false;
#endif
    }
    
    bool CIrrDeviceiOS::isWindowFocused() const
    {
#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
		CIrrDelegateiOS* delegate = [UIApplication sharedApplication].delegate;
		
		return [delegate hasFocus];
#else
		return false;
#endif
    }
    
    bool CIrrDeviceiOS::isWindowMinimized() const
    {
#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
		CIrrDelegateiOS* delegate = [UIApplication sharedApplication].delegate;
		
		return ![delegate isActive];
#else
		return false;
#endif
    }

    bool CIrrDeviceiOS::present(video::IImage* image, void * windowId, core::rect<s32>* src)
    {
        return false;
    }

    void CIrrDeviceiOS::closeDevice()
    {
        CFRunLoopStop(CFRunLoopGetMain());
		
		Close = true;
        yield();//changed
    }

    void CIrrDeviceiOS::setResizable(bool resize)
    {
    }

    void CIrrDeviceiOS::minimizeWindow()
    {
    }

    void CIrrDeviceiOS::maximizeWindow()
    {
    }

    void CIrrDeviceiOS::restoreWindow()
    {
    }
    
    core::position2di CIrrDeviceiOS::getWindowPosition()
    {
        return core::position2di(0, 0);
    }

    /* changed:
    bool CIrrDeviceiOS::activateAccelerometer(float updateInterval)
    {
        bool status = false;
        
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        CMMotionManager* motionManager = dataStorage->MotionManager;
        
        if (motionManager.isAccelerometerAvailable)
        {
            if (!motionManager.isAccelerometerActive)
            {
                motionManager.accelerometerUpdateInterval = updateInterval;
                [motionManager startAccelerometerUpdates];
            }
            
            status = true;
        }
        
        return status;
    }

    bool CIrrDeviceiOS::deactivateAccelerometer()
    {
        bool status = false;
        
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        CMMotionManager* motionManager = dataStorage->MotionManager;
        
        if (motionManager.isAccelerometerAvailable)
        {
            if (motionManager.isAccelerometerActive)
                [motionManager stopAccelerometerUpdates];
            
            status = true;
        }
        
        return status;
    }
    
    bool CIrrDeviceiOS::isAccelerometerActive()
    {
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);

        return (dataStorage->MotionManager.isAccelerometerActive);
    }

    bool CIrrDeviceiOS::isAccelerometerAvailable()
    {
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        
        return (dataStorage->MotionManager.isAccelerometerAvailable);
    }
    
    bool CIrrDeviceiOS::activateGyroscope(float updateInterval)
    {
        bool status = false;
        
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        CMMotionManager* motionManager = dataStorage->MotionManager;
        
        if (motionManager.isGyroAvailable)
        {
            if (!motionManager.isGyroActive)
            {
                motionManager.gyroUpdateInterval = updateInterval;
                [motionManager startGyroUpdates];
            }
            
            status = true;
        }
        
        return status;
    }

    bool CIrrDeviceiOS::deactivateGyroscope()
    {
        bool status = false;
        
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        CMMotionManager* motionManager = dataStorage->MotionManager;
        
        if (motionManager.isGyroAvailable)
        {
            if (motionManager.isGyroActive)
                [motionManager stopGyroUpdates];
            
            status = true;
        }
        
        return status;
    }
    
    bool CIrrDeviceiOS::isGyroscopeActive()
    {
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        
        return (dataStorage->MotionManager.isGyroActive);
    }

    bool CIrrDeviceiOS::isGyroscopeAvailable()
    {
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        
        return (dataStorage->MotionManager.isGyroAvailable);
    }
    
    bool CIrrDeviceiOS::activateDeviceMotion(float updateInterval)
    {
        bool status = false;
        
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        CMMotionManager* motionManager = dataStorage->MotionManager;
        
        if (motionManager.isDeviceMotionAvailable)
        {
            if (!motionManager.isDeviceMotionActive)
            {
                dataStorage->ReferenceAttitude = nil;
                
                motionManager.deviceMotionUpdateInterval = updateInterval;
                [motionManager startDeviceMotionUpdates];
            }
            
            status = true;
        }
        
        return status;
    }
    
    bool CIrrDeviceiOS::deactivateDeviceMotion()
    {
        bool status = false;
        
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        CMMotionManager* motionManager = dataStorage->MotionManager;
        
        if (motionManager.isDeviceMotionAvailable)
        {
            if (motionManager.isDeviceMotionActive)
            {
                [motionManager stopDeviceMotionUpdates];
                
                dataStorage->ReferenceAttitude = nil;
            }
            
            status = true;
        }
        
        return status;
    }
    
    bool CIrrDeviceiOS::isDeviceMotionActive()
    {
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        
        return (dataStorage->MotionManager.isDeviceMotionActive);
    }
    
    bool CIrrDeviceiOS::isDeviceMotionAvailable()
    {
        SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
        
        return (dataStorage->MotionManager.isDeviceMotionAvailable);
    }
    */
    E_DEVICE_TYPE CIrrDeviceiOS::getType() const
    {
        return EIDT_IOS;
    }
    
    //changed:---
    
    irr::SEvent CIrrDeviceiOS::touch2MouseEvent(const irr::SEvent::STouchInput& ev){
        irr::SEvent event;
        event.EventType = EET_MOUSE_INPUT_EVENT;
        event.MouseInput.Control = false;
        event.MouseInput.Event = ev.Event==ETIE_PRESSED_DOWN?EMIE_LMOUSE_PRESSED_DOWN:(ev.Event==ETIE_LEFT_UP?EMIE_LMOUSE_LEFT_UP:(ev.Event==ETIE_MOVED?EMIE_MOUSE_MOVED:EMIE_COUNT));
        event.MouseInput.ButtonStates = ev.Event!=ETIE_LEFT_UP?EMBSM_LEFT:0;//valid for first touch event
        event.MouseInput.Shift = false;
        event.MouseInput.Wheel = 0.f;
        event.MouseInput.X = ev.X;
        event.MouseInput.Y = ev.Y;
#ifdef _IRR_MOUSE_COMPATIBILITY
        postEventFromUser(event);
        ((CCursorControl*)(CursorControl))->mevent.MouseInput.X = ev.X;
        ((CCursorControl*)(CursorControl))->mevent.MouseInput.Y = ev.Y;
#endif
        return event;
    }
    
    unsigned int CIrrDeviceiOS::getTouchIndex(void* ptr){
        unsigned int firstEmpty = touchCount;
        for(unsigned int i=0; i<touchCount; i++){
            void* touch = touch2Index[i];
            if(touch==ptr){
                return i;
            }else if(touch==NULL && firstEmpty>=touchCount){
                firstEmpty = i;
            }
        }
        firstEmpty = firstEmpty%touchCount;
        touch2Index[firstEmpty] = ptr;
        return firstEmpty;
    }
    
    unsigned int CIrrDeviceiOS::removeTouch(void* ptr){
        for(unsigned int i=0; i<touchCount; i++){
            if(touch2Index[i]==ptr){
                touch2Index[i] = NULL;
                return i;
            }
        }
        return touchCount;
    }
    
    //changed end---

    void CIrrDeviceiOS::createWindow()
    {
		if (CreationParams.DriverType != video::EDT_NULL)
		{
			SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
			
			UIView* externalView = (__bridge UIView*)CreationParams.WindowId;
			
			if (externalView == nil)
			{
				dataStorage->Window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
				dataStorage->ViewController = [[ViewController alloc] init];//[[UIViewController alloc] init]; //changed
				[(ViewController*)dataStorage->ViewController setIrrlichtDevice:this];//changed
				dataStorage->Window.rootViewController = dataStorage->ViewController;
				
				[dataStorage->Window makeKeyAndVisible];
				[(ViewController*)dataStorage->ViewController setKeyboardVisibility:false];//changed
			}
			else
			{
				dataStorage->Window = externalView.window;
				
				UIResponder* currentResponder = externalView.nextResponder;
				
				do
				{
					if ([currentResponder isKindOfClass:[UIViewController class]])
					{
						dataStorage->ViewController = (UIViewController*)currentResponder;
						
						currentResponder = nil;
					}
					else if ([currentResponder isKindOfClass:[UIView class]])
					{
						currentResponder = currentResponder.nextResponder;
					}
					else
					{
						currentResponder = nil;
						
						// Could not find view controller.
						_IRR_DEBUG_BREAK_IF(true);
					}
				}
				while (currentResponder != nil);
			}
		}
    }
    
    void CIrrDeviceiOS::createViewAndDriver()
    {
		SIrrDeviceiOSDataStorage* dataStorage = static_cast<SIrrDeviceiOSDataStorage*>(DataStorage);
		
		video::SExposedVideoData data;
		data.OpenGLiOS.Window = (__bridge void*)dataStorage->Window;
		data.OpenGLiOS.ViewController = (__bridge void*)dataStorage->ViewController;
		
		UIView* externalView = (__bridge UIView*)CreationParams.WindowId;
		
		CGRect resolution = (externalView == nil) ? [[UIScreen mainScreen] bounds] : externalView.bounds;
        //changed
        //Required for full resolution:
        float scale = (externalView == nil) ? [[UIScreen mainScreen] scale] : 1.f;
        resolution.size.width *= scale;
        resolution.size.height *= scale;
        //changed end

        switch (CreationParams.DriverType)
        {
            case video::EDT_OGLES1:
#ifdef _IRR_COMPILE_WITH_OGLES1_
                {
					CIrrViewEAGLiOS* view = [[CIrrViewEAGLiOS alloc] initWithFrame:resolution forDevice:this];
					CreationParams.WindowSize = core::dimension2d<u32>(view.frame.size.width, view.frame.size.height);
					
					dataStorage->View = view;
					data.OpenGLiOS.View = (__bridge void*)view;

                    ContextManager = new video::CEAGLManager();
                    ContextManager->initialize(CreationParams, data);
					
                    VideoDriver = video::createOGLES1Driver(CreationParams, FileSystem, ContextManager);
                    
                    if (!VideoDriver)
                        os::Printer::log("Could not create OpenGL ES 1.x driver.", ELL_ERROR);
                }
#else
                os::Printer::log("No OpenGL ES 1.x support compiled in.", ELL_ERROR);
#endif
                break;
				
			case video::EDT_OGLES2:
#ifdef _IRR_COMPILE_WITH_OGLES2_
				{
					CIrrViewEAGLiOS* view = [[CIrrViewEAGLiOS alloc] initWithFrame:resolution forDevice:this];
                    //changed
                    //if ([view respondsToSelector:@selector(setContentScaleFactor:)])
                    //    view.ContentScaleFactor = [[UIScreen mainScreen] scale];
                    //changed end
					CreationParams.WindowSize = core::dimension2d<u32>(view.frame.size.width, view.frame.size.height);
                    printf("CreationParams.WindowSize: %i %i\n", CreationParams.WindowSize.Width, CreationParams.WindowSize.Height);
				
					dataStorage->View = view;
					data.OpenGLiOS.View = (__bridge void*)view;
				
					ContextManager = new video::CEAGLManager();
					ContextManager->initialize(CreationParams, data);
				
					VideoDriver = video::createOGLES2Driver(CreationParams, FileSystem, ContextManager);
				
					if (!VideoDriver)
						os::Printer::log("Could not create OpenGL ES 2.x driver.", ELL_ERROR);
				}
#else
				os::Printer::log("No OpenGL ES 2.x support compiled in.", ELL_ERROR);
#endif
				break;
				
            case video::EDT_SOFTWARE:
            case video::EDT_BURNINGSVIDEO:
            case video::DEPRECATED_EDT_DIRECT3D8_NO_LONGER_EXISTS:
            case video::EDT_DIRECT3D9:
            case video::EDT_OPENGL:
                os::Printer::log("This driver is not available in iOS. Try OpenGL ES.", ELL_ERROR);
                break;
                
            case video::EDT_NULL:
                VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
                break;
                
            default:
                os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
                break;
        }
		
		if (externalView == nil)
			dataStorage->ViewController.view = dataStorage->View;
		else
			[externalView addSubview:dataStorage->View];
	}
}

#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
int main(int argc, char** argv)
{
    int result = UIApplicationMain(argc, argv, 0, NSStringFromClass([CIrrDelegateiOS class]));
    
    return result;
}
#endif

#endif
