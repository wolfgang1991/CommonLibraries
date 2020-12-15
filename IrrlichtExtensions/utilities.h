#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#include <utf8.h>

#include <rect.h>
#include <SColor.h>
#include <vector2d.h>
#include <vector3d.h>

#include <string>

#include "ForwardDeclarations.h"

template <typename TScalar>
std::ostream& operator<<(std::ostream &out, const irr::core::vector2d<TScalar>& v){
	return out << v.X << ", " << v.Y;
}

template <typename TScalar>
std::ostream& operator<<(std::ostream &out, const irr::core::vector3d<TScalar>& v){
	return out << v.X << ", " << v.Y << ", " << v.Z;
}

template <typename TScalar>
std::ostream& operator<<(std::ostream &out, const irr::core::rect<TScalar>& v){
	return out << v.UpperLeftCorner << ", " << v.LowerRightCorner;
}

class IniFile;

void readIniWithAssetSupport(irr::io::IFileSystem* fsys, const std::string& file, IniFile& ini);

bool isOverlayedBySingleGUIElement(irr::gui::IGUIElement* ele, const irr::core::rect<irr::s32>& rectangle);

bool isOverlayedBySingleGUIElement(irr::gui::IGUIElement* ele, const irr::core::vector2d<irr::s32>& pos);

//! checks all children recursively
bool isOverlayedByGUIElements(irr::gui::IGUIElement* root, const irr::core::rect<irr::s32>& rectangle);

//! checks all children recursively
bool isOverlayedByGUIElements(irr::gui::IGUIElement* root, const irr::core::vector2d<irr::s32>& pos);

irr::video::SColor getInterpolatedColor(irr::video::IImage* img, double sx, double sy);

//! moves a rectange in a way that it is always inside limiting
irr::core::rect<irr::s32> limitRect(irr::core::rect<irr::s32> r, const irr::core::rect<irr::s32>& limiting);

//! fills a container with fillWithValue until it reaches the desired size. The ctr reference is returned again.
template <typename TContainer>
TContainer& fillToSize(TContainer& ctr, uint32_t desiredSize, typename TContainer::value_type fillWithValue = 0){
	for(uint32_t i=ctr.size(); i<desiredSize; i++){
		ctr.push_back(fillWithValue);//Complexity depends on container (e.g. std::vector: O(n) amortized, std::list O(n))
	}
	return ctr;
}

//! Helper function to remove all hardware buffers associated with a mesh
void removeMeshHardwareBuffers(irr::video::IVideoDriver* driver, irr::scene::IMesh* mesh);

//! Shows a full screen message, and returns the result of IrrlichtDevice::run()
bool showState(irr::IrrlichtDevice* device, const wchar_t* txt);

#endif
