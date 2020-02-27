//changed
#ifndef IOSVIEWCONTROLLER_H_INCLUDED
#define IOSVIEWCONTROLLER_H_INCLUDED

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_IOS_DEVICE_
#import <UIKit/UIKit.h>

namespace irr
{
    class IrrlichtDevice;
}

// Exposing this interface is required to make it possible to alter settings such as AutoRotate without modifing the Irrlicht Engine or creating an own view with own ViewController
@interface ViewController : UIViewController <UIKeyInput>

- (void)setShouldAutoRotate:(bool)autoRotate;

- (void)setPreferStatusBarHidden:(bool)statusBarHidden;

- (void)setKeyboardVisibility:(bool)visibility;

- (void)setIrrlichtDevice:(irr::IrrlichtDevice*)device;

@property(nonatomic) UITextAutocorrectionType autocorrectionType;

@end

#endif

#endif
//changed end
