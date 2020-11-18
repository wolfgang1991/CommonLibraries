#List of object files without path
_LINKOBJ =  main.o

COMMONLIBPATH = ../..
SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -D_DEBUG=$(DEBUG) -Wall -I$(COMMONLIBPATH)/Irrlicht/include -I/usr/X11R6/include -I. -I$(COMMONLIBPATH)/Common -I$(COMMONLIBPATH)/IrrlichtExtensions
LIBFLAGS = -L$(COMMONLIBPATH)/IrrlichtExtensions -lIrrlichtExtensions -L$(COMMONLIBPATH)/Common -lCommon
EXECFILE = ./GUIElementTests
USEROPTIM = 

all: all_linux

include $(COMMONLIBPATH)/MakefileCommon

build_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" DEBUG=$(DEBUG)
	cd $(COMMONLIBPATH)/IrrlichtExtensions && "$(MAKE)" DEBUG=$(DEBUG)
	
clean_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" clean
	cd $(COMMONLIBPATH)/IrrlichtExtensions && "$(MAKE)" clean

