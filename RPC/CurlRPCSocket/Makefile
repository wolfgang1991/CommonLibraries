ifndef CURL_PATH
$(error CURL_PATH is not set)
endif

#List of object files without path
_LINKOBJ = CurlRPCSocket.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I.. -I../jsonrpcpp -I../../Common -I. -I$(CURL_PATH) -D_DEBUG=$(DEBUG)
STATIC_LIB = libCurlRPCSocket.a
USEROPTIM = 

all: all_lib

include ../../MakefileLibCommon

build_deps:
	cd ../../Common && "$(MAKE)" DEBUG=$(DEBUG)

clean_deps:
	cd ../../Common && "$(MAKE)" clean
