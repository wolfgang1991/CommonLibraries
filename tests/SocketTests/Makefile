#List of object files without path
_LINKOBJ = main.o

COMMONLIBPATH = ../..
SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -D_DEBUG=$(DEBUG) -Wall -I. -I$(COMMONLIBPATH)/Common
COMMONLIBFLAGS = -L$(COMMONLIBPATH)/Common -lCommon
LINUX_LIBFLAGS = -pthread
WIN32_LIBFLAGS = -lwinmm -lws2_32 -static -pthread
EXECFILE = ./SocketTests
USEROPTIM = 

all: all_linux

include $(COMMONLIBPATH)/MakefileConsoleCommon

build_deps:
	cd $(COMMONLIBPATH)/Common && "$(MAKE)" DEBUG=$(DEBUG)
	
clean_deps:
	#cd $(COMMONLIBPATH)/Common && $(MAKE) clean

