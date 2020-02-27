#List of object files without path
_LINKOBJ = main.o

COMMONLIBPATH = ../..
SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -D_DEBUG=$(DEBUG) -Wall -I$(COMMONLIBPATH)/Irrlicht/include -I/usr/X11R6/include -I. -I$(COMMONLIBPATH)/Common
LIBFLAGS = -L$(COMMONLIBPATH)/Common -lCommon -pthread
EXECFILE = ./SocketTests
USEROPTIM = 

all: all_linux

include $(COMMONLIBPATH)/MakefileCommon

build_deps:
	cd $(COMMONLIBPATH)/Common && $(MAKE) DEBUG=$(DEBUG)
	
clean_deps:
	cd $(COMMONLIBPATH)/Common && $(MAKE) clean

