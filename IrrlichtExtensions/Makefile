#List of object files without path
_LINKOBJ = 	NumberEditBox.o GUI.o utilities.o Drawer2D.o CMBox.o font.o AggregateGUIElement.o IAggregatableGUIElement.o IExtendableSkin.o ScrollBarSkinExtension.o ScrollBar.o BeautifulGUIImage.o AggregateSkinExtension.o \
				DraggableGUIElement.o DragPlaceGUIElement.o FlexibleFont.o Transformation2DHelpers.o LoadSaveSettingsDialog.o AggregatableGUIElementAdapter.o EditBoxDialog.o RectangleGradientDescent.o GUIHelp.o UnicodeCfgParser.o \
				ConstantLanguagePhrases.o ProgressBar.o BeautifulGUIText.o NotificationBox.o InputSystem.o KeyInput.o TouchKey.o TouchKeyboard.o AMLGUIElement.o BeautifulGUIButton.o ItemSelectElement.o FileSystemItemOrganizer.o ChooseFromListDialog.o CommonIniEditor.o ColorSelector.o \
				ZoomBarGUIElement.o

SRCDIR = .
OBJDIR = $(SRCDIR)/obj
CPPFLAGS = -Wall -I../Irrlicht/include -I../Common -I. -D_DEBUG=$(DEBUG)
STATIC_LIB = libIrrlichtExtensions.a
USEROPTIM = 

all: all_lib

include ../MakefileLibCommon

build_deps:
	cd ../Common && "$(MAKE)" DEBUG=$(DEBUG)

clean_deps:
	cd ../Common && "$(MAKE)" clean
