// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IRR_DEVICE_ANDROID_H_INCLUDED__
#define __C_IRR_DEVICE_ANDROID_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_ANDROID_DEVICE_

#include "CIrrDeviceStub.h"
#include "IrrlichtDevice.h"
#include "IImagePresenter.h"
#include "ICursorControl.h"

#include <android/sensor.h>
#include <android_native_app_glue.h>

namespace irr
{
	class CIrrDeviceAndroid : public CIrrDeviceStub, video::IImagePresenter
	{
	public:
		CIrrDeviceAndroid(const SIrrlichtCreationParameters& param);

		virtual ~CIrrDeviceAndroid();

		virtual bool run();

		virtual void yield();

		virtual void sleep(u32 timeMs, bool pauseTimer = false);

		virtual void setWindowCaption(const wchar_t* text);

		virtual bool present(video::IImage* surface, void* windowId, core::rect<s32>* srcClip);

		virtual bool isWindowActive() const;

		virtual bool isWindowFocused() const;

		virtual bool isWindowMinimized() const;

		virtual void closeDevice();

		virtual void setResizable(bool resize = false);

		virtual void minimizeWindow();

		virtual void maximizeWindow();

		virtual void restoreWindow();

		virtual core::position2di getWindowPosition();

		virtual E_DEVICE_TYPE getType() const;

		virtual bool activateAccelerometer(float updateInterval);

		virtual bool deactivateAccelerometer();

		virtual bool isAccelerometerActive();

		virtual bool isAccelerometerAvailable();

		virtual bool activateGyroscope(float updateInterval);

		virtual bool deactivateGyroscope();

		virtual bool isGyroscopeActive();

		virtual bool isGyroscopeAvailable();

	private:
	
		bool isInMultiWindowMode();//changed
	
		static void initSurface(android_app* app);//changed
		
		static void destroySurface(android_app* app);//changed
		
		void checkAndHandleScreenChange();//changed

		static void handleAndroidCommand(android_app* app, int32_t cmd);

		static s32 handleInput(android_app* app, AInputEvent* event);

		void createDriver();

		void createKeyMap();

		video::SExposedVideoData& getExposedVideoData();

		android_app* Android;
		ASensorManager* SensorManager;
		ASensorEventQueue* SensorEventQueue;
		const ASensor* Accelerometer;
		const ASensor* Gyroscope;

		bool Focused;
		bool Initialized;
		bool InitAborted;//true if init aborted before driver init (required to prevent splash screen hangs) //changed 
		bool Paused;
		bool Stopped;//changed
		bool IsSplitScreen;//changed
		bool IsWindowInitialized;

		JNIEnv* JNIEnvAttachedToVM;
		jmethodID isInMultiWindowModeID;//changed

		video::SExposedVideoData ExposedVideoData;

		core::array<EKEY_CODE> KeyMap;

		//changed
		s32 ButtonStates;//modified by down and release events
		//changed end

		//changed:

		//! Android Cursor Control
		class CCursorControl : public gui::ICursorControl
		{
		private:
			core::rect<s32> refrect;
			core::dimension2d<u32> sSize;
			IrrlichtDevice* Device;
			core::position2d<s32> curPos;//only to support the stupid reference in getPosition

		public:
			SEvent mevent;

			CCursorControl(IrrlichtDevice* device, core::dimension2d<u32> ssize){
				mevent.MouseInput.ButtonStates = 0;
				mevent.MouseInput.Control = false;
				mevent.MouseInput.Event = EMIE_MOUSE_MOVED;
				mevent.MouseInput.Shift = false;
				mevent.MouseInput.Wheel = 0.0f;
				mevent.MouseInput.X = 0;
				mevent.MouseInput.Y = 0;
				sSize = ssize;
				refrect = core::rect<s32>(0,0,sSize.Width,sSize.Height);
				Device = device;
			}
			
			void OnResize(const irr::core::dimension2d<u32>& size){
				if(refrect.getWidth()==sSize.Width && refrect.getHeight()==sSize.Height){//unchanged by the user => use new screenrect
					sSize = size;
					setReferenceRect();
				}else{
					sSize = size;
				}
			}

			virtual void setVisible(bool visible){}//TODO: external mouse cursor visibility

			virtual bool isVisible() const{return false;}

			virtual void setPosition(const core::position2d<f32> &pos){
				setPosition(pos.X, pos.Y);
			}

			virtual void setPosition(f32 x, f32 y){
				setPosition((s32)(x*refrect.getWidth()), (s32)(y*refrect.getHeight()));
			}

			virtual void setPosition(const core::position2d<s32> &pos){
				setPosition(pos.X, pos.Y);
			}

			virtual void setPosition(s32 x, s32 y){
				#ifdef _IRR_ANDROID_MOUSE_COMPATIBILITY
				mevent.MouseInput.X = x+refrect.UpperLeftCorner.X;
				mevent.MouseInput.Y = y+refrect.UpperLeftCorner.Y;
				Device->postEventFromUser(mevent);
				#endif
			}

			virtual const core::position2d<s32>& getPosition(bool updateCursor=true){
				curPos = core::position2d<s32>(mevent.MouseInput.X-refrect.UpperLeftCorner.X, mevent.MouseInput.Y-refrect.UpperLeftCorner.Y);
				return curPos;
			}

			virtual core::position2d<f32> getRelativePosition(bool updateCursor=true){
				return core::position2d<f32>((f32)(mevent.MouseInput.X-refrect.UpperLeftCorner.X)/refrect.getWidth(), (f32)(mevent.MouseInput.Y-refrect.UpperLeftCorner.Y)/refrect.getHeight());
			}

			virtual void setReferenceRect(core::rect<s32>* rect=0){
				if(rect){
					refrect = core::rect<s32>(*rect);
				}else{
					refrect = core::rect<s32>(0,0,sSize.Width,sSize.Height);
				}
			}
		};

	//changed end
	};

} // end namespace irr

#endif // _IRR_COMPILE_WITH_ANDROID_DEVICE_
#endif // __C_IRR_DEVICE_ANDROID_H_INCLUDED__
