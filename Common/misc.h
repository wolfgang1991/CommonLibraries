#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <ostream>

std::ostream& hex_dump(std::ostream& os, const void *buffer, std::size_t bufsize, bool showPrintableChars = true);

#endif
