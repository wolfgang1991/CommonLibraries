#List of object files without path
_LINKOBJ = JSONRPC2Client.o JSONRPC2Server.o JSONParser.o RequestBasedJSONRPC2Client.o RequestBasedJSONRPC2ServerHelpers.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I.. -I../jsonrpcpp -I../../Common -I. -D_DEBUG=$(DEBUG)
STATIC_LIB = libJSONRPC2.a
USEROPTIM = 

all: all_lib

include ../../MakefileLibCommon

build_deps:
	cd ../../Common && "$(MAKE)" DEBUG=$(DEBUG)

clean_deps:
	cd ../../Common && "$(MAKE)" clean
