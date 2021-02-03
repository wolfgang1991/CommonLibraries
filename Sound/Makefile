#List of object files without path
_LINKOBJ = 	SineWaveSoundSource.o SoundManager.o AlsaSoundDriver.o StaticWaveFileSource.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I../Irrlicht/include -I../Common -I. -D_DEBUG=$(DEBUG)
STATIC_LIB = libSound.a
USEROPTIM = 

all: all_lib

include ../MakefileLibCommon

build_deps:
	cd ../Common && "$(MAKE)" DEBUG=$(DEBUG)

clean_deps:
	cd ../Common && "$(MAKE)" clean
