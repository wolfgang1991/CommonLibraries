#List of object files without path
_LINKOBJ =  main.o

COMMONLIBPATH = ../..
SRCDIR = .
OBJDIR = $(SRCDIR)/obj

OPENCVDIR = $(COMMONLIBPATH)/../OpenCV/opencv#must be altered depending on your OpenCV checkout
include $(COMMONLIBPATH)/MakefileOpenCVCommon

CPPFLAGS = -D_DEBUG=$(DEBUG) -Wall -I$(COMMONLIBPATH)/Irrlicht/include -I/usr/X11R6/include -I. -I$(COMMONLIBPATH)/IrrlichtOpenCVGlue -I$(COMMONLIBPATH)/Common -I$(COMMONLIBPATH)/PathTransform $(OPENCVINCLUDES)
COMMONLIBFLAGS = -L$(COMMONLIBPATH)/IrrlichtOpenCVGlue -lIrrlichtOpenCVGlue -L$(COMMONLIBPATH)/IrrlichtExtensions -lIrrlichtExtensions -L$(COMMONLIBPATH)/Common -lCommon
LINUX_LIBFLAGS = $(OPENCVLIBS)
WIN32_LIBFLAGS = $(OPENCVLIBS_WIN32)

EXECFILE = ./PathTransformTest
USEROPTIM = 

all: all_linux

include $(COMMONLIBPATH)/MakefileCommon

build_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" DEBUG=$(DEBUG)
	cd $(COMMONLIBPATH)/IrrlichtExtensions && "$(MAKE)" DEBUG=$(DEBUG)
	cd $(COMMONLIBPATH)/IrrlichtOpenCVGlue && "$(MAKE)" DEBUG=$(DEBUG)
	
clean_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" clean
	cd $(COMMONLIBPATH)/IrrlichtExtensions && "$(MAKE)" clean
	cd $(COMMONLIBPATH)/IrrlichtOpenCVGlue && "$(MAKE)" clean
