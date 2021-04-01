#ifndef FORWARDDECLARATIONS_H_INCLUDED
#define FORWARDDECLARATIONS_H_INCLUDED

namespace irr{
	class IrrlichtDevice;
	struct SEvent;
	namespace gui{
		class ICursorControl;
		class IGUIEnvironment;
		class IGUIWindow;
		class IGUIButton;
		class IGUIStaticText;
		class IGUIScrollBar;
		class IGUICheckBox;
		class IGUIComboBox;
		class IGUIFont;
		class IGUISkin;
		class IGUIElement;
		class IGUIEditBox;
		class IGUIListBox;
		class IGUIImage;
	}
	namespace video{
		class IVideoDriver;
		class ITexture;
		class IImage;
		class IVideoDriver;
		class IImageLoader;
		class IImageWriter;
	}
	namespace scene{
		class ISceneManager;
		class ISceneNode;
		class ICameraSceneNode;
		class SMesh;
		class IMesh;
		class IMeshSceneNode;
		class IBillboardSceneNode;
	}
	namespace io{
		class IFileSystem;
		class IReadFile;
	}
}

class GUI;
class Context;
class CMBox;

#endif
