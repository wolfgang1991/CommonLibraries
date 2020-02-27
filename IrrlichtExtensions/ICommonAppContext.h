#ifndef ICommonAppContext_H_INCLUDED
#define ICommonAppContext_H_INCLUDED

#include <irrTypes.h>
#include <IEventReceiver.h>

#include <string>

class Drawer2D;
class FlexibleFont;

namespace irr{
	class IrrlichtDevice;
}

//! Interface for EventReceivers which support the registration of multiple other EventReceivers
class IMultiEventReceiver : public irr::IEventReceiver{
	
	public:
	
	virtual ~IMultiEventReceiver(){}
	
	virtual void addSubEventReceiver(irr::IEventReceiver* rcv) = 0;

	virtual void removeSubEventReceiver(irr::IEventReceiver* rcv) = 0;
	
};

//! Interface to provide stuff which is usually required in all Irrlicht based apps with nice stuff such as FlexibleFont and Drawer2D
//! This is useful since the 'true' application context may contain much more application specific stuff which we don't need for many things.
//! This ensures the portability of code using the nice stuff without having to know anything about the specific application.
class ICommonAppContext{
	
	public:
	
	virtual ~ICommonAppContext(){}
	
	virtual IMultiEventReceiver* getEventReceiver() = 0;
	
	virtual Drawer2D* getDrawer() = 0;
	
	virtual irr::IrrlichtDevice* getIrrlichtDevice() = 0;
	
	virtual FlexibleFont* getFlexibleFont() = 0;
	
	//! useful to programmatically create buttons with a reasonable size
	virtual irr::s32 getRecommendedButtonWidth() = 0;
	
	//! useful to programmatically create buttons with a reasonable size
	virtual irr::s32 getRecommendedButtonHeight() = 0;
	
	//! returns the full path to a given file/"virtual" path (application/OS dependent)
	virtual std::string getPath(const std::string& file) = 0;
	
	//! sets the visibility of the OS's touch keyboard if applicable
	virtual void setSoftInputVisibility(bool visible) = 0;
	
	//! open an URL in the preferred browser of the OS
	virtual void gotoURL(const std::string& url) = 0;
	
};

#endif
