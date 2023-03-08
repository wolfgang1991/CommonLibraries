# add source files to library
add_library(IrrlichtExtensions AggregateGUIElement.cpp
        AggregatableGUIElementAdapter.cpp AggregateSkinExtension.cpp BeautifulGUIButton.cpp
        BeautifulGUIImage.cpp BeautifulGUIText.cpp ChooseFromListDialog.cpp CMBox.cpp ColorSelector.cpp
        ConstantLanguagePhrases.cpp CommonIniEditor.cpp Drawer2D.cpp
        EditBoxDialog.cpp FileSystemItemOrganizer.cpp
        FlexibleFont.cpp font.cpp GUI.cpp IAggregatableGUIElement.cpp
        IExtendableSkin.cpp InputSystem.cpp
        ItemSelectElement.cpp KeyInput.cpp ScrollBar.cpp ScrollBarSkinExtension.cpp
        UnicodeCfgParser.cpp utilities.cpp TouchKey.cpp TouchKeyboard.cpp Transformation2DHelpers.cpp
		Triangulate.cpp BeautifulCheckBox.cpp)

# include needed header file directories
include_directories(../Irrlicht/include ../Common)

# interface library for targets
target_include_directories(IrrlichtExtensions INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})