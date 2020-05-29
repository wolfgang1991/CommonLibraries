CommonLibraries
================

These libraries provide extensions that may especially be useful when developing cross-platform C++ applications with the Irrlicht 3D Engine.

It has been tested with Android, iOS and Linux. Linux Makefiles are included. For Android and iOS please add the respective source files to your Android.mk or Xcode project.

It is split into the following domains:

Common: Irrlicht independent useful functions such as:
* Simple Socket Abstraction Layer
* Simple Thread Abstraction Layer
* XML parser
* Ini parser
* Serial Port Abstraction Layer
* Simple Matrix/Vector Math Implementation
* etc..

Irrlicht: Fork from the OGLES branch of the Irrlicht 3D Engine (http://irrlicht.sourceforge.net/), some changes have been made for touch screen optimization .
For simplifying future Irrlicht updates regarding the changes in this fork the svn directory is available in Irrlicht/.svn.tar.gz .

IrrlichtExtensions:
* FlexibleFont: BMFont loader + Signed Distance Field Font implementation + arbitrary font transformations + effects (SDFF fonts can be created with https://github.com/libgdx/libgdx/wiki/Hiero )
* Drawer2D: 2D Drawing functions with transformations, polygons, etc..
* Additional GUI elements that are usable with a touch screen (more beautiful and more efficient than the built-in Irrlicht GUI elements, some require the usage of a skin that inherits from / can safely be casted to an IExtendableSkin).
* AML ("App Markup Language", similar to HTML) parser: Translates AML files into GUI elements.
* Platform independent touch screen keyboard
* etc..

IrrlichtOpenCVGlue: Some glue code to use OpenCV with Irrlicht

PathTransform: Transformations of paths (containers of vectors) using transformations determined by different least squares approximations from matched points.

RPC: A JSON parser and a JSON-RPC implementation (client and server)

