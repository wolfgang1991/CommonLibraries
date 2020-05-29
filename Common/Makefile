#List of object files without path
_LINKOBJ = IniFile.o IniIterator.o IniParser.o timing.o StringHelpers.o SimpleSockets.o CRC32.o Threading.o AParallelFunction.o XMLParser.o utf8.o RS232.o serial.o

_C_LINKOBJ = serial.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I. -D_DEBUG=$(DEBUG)
CFLAGS = $(CPPFLAGS)
STATIC_LIB = libCommon.a
USEROPTIM = 

all: all_lib

include ../MakefileLibCommon

build_deps:
	#no dependencies

clean_deps:
	#no dependencies
