#List of object files without path
_LINKOBJ = main.o

COMMONLIBPATH = ../..
SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -D_DEBUG=$(DEBUG) -Wall -I. -I$(COMMONLIBPATH)/Common -I$(COMMONLIBPATH)/uCCommon
COMMONLIBFLAGS = -L$(COMMONLIBPATH)/Common -lCommon
EXECFILE = ./uCTests
USEROPTIM = 

all: all_linux

include $(COMMONLIBPATH)/MakefileConsoleCommon

build_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" DEBUG=$(DEBUG)
	
clean_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" clean

