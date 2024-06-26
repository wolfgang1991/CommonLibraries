#List of object files without path
_LINKOBJ = IniFile.o IniIterator.o IniParser.o timing.o StringHelpers.o SimpleSockets.o CRC32.o Threading.o AParallelFunction.o \
XMLParser.o utf8.o Serial.o misc.o ConcurrentCommunicationEndpoint.o NamedPipes.o ZSocket.o SSLSocket.o RTPSender.o

_C_LINKOBJ = cserial.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I. -D_DEBUG=$(DEBUG)
CFLAGS = $(CPPFLAGS)
STATIC_LIB = libCommon.a
USEROPTIM = 

ifeq ($(NO_ZLIB),1)
CPPFLAGS += -DNO_ZLIB=$(NO_ZLIB)
endif

ifeq ($(NO_OPENSSL),1)
CPPFLAGS += -DNO_OPENSSL=$(NO_OPENSSL)
endif

all: all_lib

include ../MakefileLibCommon

build_deps:
	#no dependencies

clean_deps:
	#no dependencies
