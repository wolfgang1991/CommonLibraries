// Copyright (C) 2002-2008 Nikolaus Gebhardt
// Copyright (C) 2008 Redshift Software, Inc.
// Copyright (C) 2012-2015 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IRR_DEVICE_IOS_H_INCLUDED__
#define __C_IRR_DEVICE_IOS_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_IOS_DEVICE_

#include "CIrrDeviceStub.h"
#include "IrrlichtDevice.h"
#include "IImagePresenter.h"

#include "irrList.h"  //changed

namespace irr
{

	class CIrrDeviceiOS : public CIrrDeviceStub, public video::IImagePresenter
	{
	public:
		CIrrDeviceiOS(const SIrrlichtCreationParameters& params);
		virtual ~CIrrDeviceiOS();

		virtual bool run() _IRR_OVERRIDE_;
		virtual void yield() _IRR_OVERRIDE_;
		virtual void sleep(u32 timeMs, bool pauseTimer) _IRR_OVERRIDE_;

		virtual void setWindowCaption(const wchar_t* text) _IRR_OVERRIDE_;

		virtual bool isWindowActive() const _IRR_OVERRIDE_;
		virtual bool isWindowFocused() const _IRR_OVERRIDE_;
		virtual bool isWindowMinimized() const _IRR_OVERRIDE_;

		virtual bool present(video::IImage* surface, void * windowId = 0, core::rect<s32>* src = 0) _IRR_OVERRIDE_;

		virtual void closeDevice() _IRR_OVERRIDE_;

		virtual void setResizable(bool resize = false) _IRR_OVERRIDE_;

		virtual void minimizeWindow() _IRR_OVERRIDE_;
		virtual void maximizeWindow() _IRR_OVERRIDE_;
		virtual void restoreWindow() _IRR_OVERRIDE_;

		virtual core::position2di getWindowPosition() _IRR_OVERRIDE_;

        /* changed:
        virtual bool activateAccelerometer(float updateInterval = 0.016666f) _IRR_OVERRIDE_;
        virtual bool deactivateAccelerometer() _IRR_OVERRIDE_;
        virtual bool isAccelerometerActive() _IRR_OVERRIDE_;
        virtual bool isAccelerometerAvailable() _IRR_OVERRIDE_;
        virtual bool activateGyroscope(float updateInterval = 0.016666f) _IRR_OVERRIDE_;
        virtual bool deactivateGyroscope() _IRR_OVERRIDE_;
        virtual bool isGyroscopeActive() _IRR_OVERRIDE_;
        virtual bool isGyroscopeAvailable() _IRR_OVERRIDE_;
        virtual bool activateDeviceMotion(float updateInterval = 0.016666f) _IRR_OVERRIDE_;
        virtual bool deactivateDeviceMotion() _IRR_OVERRIDE_;
        virtual bool isDeviceMotionActive() _IRR_OVERRIDE_;
        virtual bool isDeviceMotionAvailable() _IRR_OVERRIDE_;
        */

        virtual E_DEVICE_TYPE getType() const _IRR_OVERRIDE_;
        
        //changed:---
        
        static const unsigned int touchCount = 20;
        void* touch2Index[touchCount];
        //irr::core::list<void*> touch2Index;
        
        //! return index and pushes the new touch if applicable
        unsigned int getTouchIndex(void* ptr);
        
        //! returns old index and pops the old touch
        unsigned int removeTouch(void* ptr);
        
        //! Translate TouchEvent to mousevent, post it and update cursor control
        irr::SEvent touch2MouseEvent(const irr::SEvent::STouchInput& ev);
        
        //! iOS CursorControl
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
#ifdef _IRR_MOUSE_COMPATIBILITY
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
        //changed end---

	private:
        void createWindow();
        void createViewAndDriver();
        
        void* DataStorage;
		
		bool Close;
	};

}

#ifdef _IRR_COMPILE_WITH_IOS_BUILTIN_MAIN_
extern void irrlicht_main();
#endif

#endif
#endif
