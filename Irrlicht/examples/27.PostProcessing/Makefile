# Makefile for Irrlicht Examples
# It's usually sufficient to change just the target name and source file list
# and be sure that CXX is set to a valid compiler
Target = 27.PostProcessing
Sources = main.cpp

# general compiler settings
CPPFLAGS = -I../../include -I/usr/X11R6/include
CXXFLAGS = -O3 -ffast-math
#CXXFLAGS = -g -Wall

#default target is Linux
all: all_linux

ifeq ($(HOSTTYPE), x86_64)
LIBSELECT=64
endif

# target specific settings
# make WAYLAND=1 uses the Wayland device (_IRR_LINUX_WAYLAND_) instead of X11 (the engine must be built the same way)
ifdef WAYLAND
all_linux: CPPFLAGS += -D_IRR_LINUX_WAYLAND_
all_linux: LDFLAGS = -L../../lib/Linux -lIrrlicht -lGL -lEGL -lGLESv2 -lwayland-client -lwayland-egl -lwayland-cursor -lxkbcommon -ldecor-0
else
all_linux: LDFLAGS = -L/usr/X11R6/lib$(LIBSELECT) -L../../lib/Linux -lIrrlicht -lGL -lXxf86vm -lXext -lX11 -lXcursor -lEGL -lGLESv2
endif
all_linux clean_linux: SYSTEM=Linux
all_win32: LDFLAGS = -L../../lib/Win32-gcc -lIrrlicht -lopengl32 -lm
all_win32 clean_win32: SYSTEM=Win32-gcc
all_win32 clean_win32: SUF=.exe
# name of the binary - only valid for targets which set SYSTEM
DESTPATH = ../../bin/$(SYSTEM)/$(Target)$(SUF)

all_linux all_win32:
	$(warning Building...)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(Sources) -o $(DESTPATH) $(LDFLAGS)

clean: clean_linux clean_win32
	$(warning Cleaning...)

clean_linux clean_win32:
	@$(RM) $(DESTPATH)

.PHONY: all all_win32 clean clean_linux clean_win32
