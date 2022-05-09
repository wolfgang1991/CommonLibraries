#List of object files without path
_LINKOBJ = RPCProxyServer.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I.. -I../../Common -I. -D_DEBUG=$(DEBUG)
STATIC_LIB = libRPCProxy.a
USEROPTIM = 

all: all_lib

include ../../MakefileLibCommon

build_deps:
	cd ../../Common && "$(MAKE)" DEBUG=$(DEBUG)

clean_deps:
	cd ../../Common && "$(MAKE)" clean
