#include "ExtLib.h"

int yaz0_encode2(uint8_t* src, uint8_t* dest, int uncompressedSize);

void Yaz_Decode(uint8_t* src, uint8_t* dst, int uncompressedSize);

int Yaz_Encode(uint8_t* src, uint8_t* dest, int srcSize);