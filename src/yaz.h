#include <ext_type.h>

void Yaz_Decode(u8* dst, u8* src, s32 uncompressedSize);
s32 Yaz_Encode(u8* dst, u8* src, s32 srcSize);
const char* Yaz_Filename(const char* file);
void Yaz_EncodeThread(const char* file);