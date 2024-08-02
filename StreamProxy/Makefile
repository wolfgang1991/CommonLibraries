#List of object files without path
_LINKOBJ = StreamProxy.o StreamProxyClient.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I../RPC -I../RPC/JSONRPC2 -I../Common -I. -D_DEBUG=$(DEBUG)
STATIC_LIB = libStreamProxy.a
USEROPTIM = 

all: all_lib

include ../MakefileLibCommon

build_deps:
	cd ../Common && "$(MAKE)" DEBUG=$(DEBUG)
	cd ../RPC/JSONRPC2 && "$(MAKE)" DEBUG=$(DEBUG)

clean_deps:
	cd ../Common && "$(MAKE)" clean
	cd ../RPC/JSONRPC2 && "$(MAKE)" clean
