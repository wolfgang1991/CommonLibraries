#List of object files without path
_LINKOBJ =  main.o

COMMONLIBPATH = ../..
SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -D_DEBUG=$(DEBUG) -Wall -I. -I$(COMMONLIBPATH)/Common -I$(COMMONLIBPATH)/Sound
COMMONLIBFLAGS = -L$(COMMONLIBPATH)/Sound -lSound -L$(COMMONLIBPATH)/Common -lCommon
EXECFILE = ./SoundTests
LINUX_LIBFLAGS = -lasound
USEROPTIM = 

all: all_linux

include $(COMMONLIBPATH)/MakefileConsoleCommon

build_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" DEBUG=$(DEBUG)
	cd $(COMMONLIBPATH)/Sound && "$(MAKE)" DEBUG=$(DEBUG)
	
clean_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" clean
	cd $(COMMONLIBPATH)/Sound && "$(MAKE)" clean

