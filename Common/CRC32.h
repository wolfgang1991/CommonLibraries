#ifndef CRC32_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

extern uint32_t crc_32_tab[];

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ ((unsigned char)octet)) & 0xff] ^ ((crc) >> 8))
 
uint32_t updateCRC32(unsigned char ch, uint32_t crc);

uint32_t crc32buf(const char* buf, size_t len);

#endif
