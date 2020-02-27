#List of object files without path
_LINKOBJ = 	IrrCVImageConversion.o ConcurrentBlobDetection.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj

#opencv stuff (must be updated if opencv is updated or other opencv modules are needed)
OPENCVDIR = ../../OpenCV/opencv#this path must be adapted to the path of your opencv checkout
include ../MakefileOpenCVCommon

CPPFLAGS = -Wall -I../Irrlicht/include -I../Common -I../IrrlichtExtensions -I. -D_DEBUG=$(DEBUG) $(OPENCVINCLUDES)
STATIC_LIB = libIrrlichtOpenCVGlue.a
USEROPTIM = 

all: all_lib

include ../MakefileLibCommon

build_deps:
	cd ../Common && $(MAKE) DEBUG=$(DEBUG)
	cd ../IrrlichtExtensions && $(MAKE) DEBUG=$(DEBUG)

clean_deps:
	cd ../Common && $(MAKE) clean
	cd ../IrrlichtExtensions && $(MAKE) clean
