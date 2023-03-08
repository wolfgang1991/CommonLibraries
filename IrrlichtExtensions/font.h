#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED

#include <irrTypes.h>

#include <string>

#include "ForwardDeclarations.h"

//! O(m*n) (m max. line length (which is constant in case of reasonable maxWidth), n #chars)
std::wstring makeWordWrappedText(const std::wstring& text, irr::u32 maxWidth, irr::gui::IGUIFont* font, bool withHyphen = true, bool removeTrailingNewLines = true);

void breakGUIElementText(irr::gui::IGUIElement* ele, irr::gui::IGUIFont* font, irr::f32 horizontalPaddingPart = 0.04f, bool withHyphen = true, bool removeTrailingNewLines = true);

#endif
