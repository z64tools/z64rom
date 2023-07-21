#include <ext_lib.h>
#include "yaz.h"

u32 z64compress_yaz(u8* output, u8* data, u32 data_size);

// decoder implementation by thakis of http://www.amnoid.de
// encoder implementation by shevious, with bug fixes by notwa

#define MAX_RUNLEN (0xFF + 0x12)

void Yaz_Decode(u8* dst, u8* src, s32 uncompressedSize) {
	s32 srcPlace = 0, dstPlace = 0; // current read/write positions
	
	u32 validBitCount = 0; // number of valid bits left in "code" byte
	u8 currCodeByte;
	
	while (dstPlace < uncompressedSize) {
		// read new "code" byte if the current one is used up
		if (validBitCount == 0) {
			currCodeByte = src[srcPlace];
			++srcPlace;
			validBitCount = 8;
		}
		
		if ((currCodeByte & 0x80) != 0) {
			// straight copy
			dst[dstPlace] = src[srcPlace];
			dstPlace++;
			srcPlace++;
		} else {
			// RLE part
			u8 byte1 = src[srcPlace];
			u8 byte2 = src[srcPlace + 1];
			srcPlace += 2;
			
			u32 dist = ((byte1 & 0xF) << 8) | byte2;
			u32 copySource = dstPlace - (dist + 1);
			
			u32 numBytes = byte1 >> 4;
			if (numBytes == 0) {
				numBytes = src[srcPlace] + 0x12;
				srcPlace++;
			} else {
				numBytes += 2;
			}
			
			// copy run
			for (u32 i = 0; i < numBytes; ++i) {
				dst[dstPlace] = dst[copySource];
				copySource++;
				dstPlace++;
			}
		}
		
		// use next bit from "code" byte
		currCodeByte <<= 1;
		validBitCount -= 1;
	}
}

s32 Yaz_Encode(u8* dst, u8* src, s32 srcSize) {
	return z64compress_yaz(dst, src, srcSize);
}

const char* Yaz_Filename(const char* file) {
	char* out = alloc(strlen(file) + strlen("rom/yaz-cache/") + strlen(".yaz") + 0x20);
	
	strcpy(out, file);
	if (strstart(out, "rom/"))
		strrep(out, "rom/", "rom/yaz-cache/");
	else if (strstart(out, "src/"))
		strrep(out, "src/", "rom/yaz-cache/");
	else
		strins(out, "rom/yaz-cache/", 0);
	strcat(out, ".yaz");
	
	return out;
}

void Yaz_EncodeThread(const char* file) {
	Memfile mem = Memfile_New();
	Memfile yaz = Memfile_New();
	const char* out = Yaz_Filename(file);
	
	Memfile_LoadBin(&mem, file);
	Memfile_Alloc(&yaz, mem.size * 3);
	
	yaz.size = Yaz_Encode(yaz.data, mem.data, mem.size);
	
	sys_mkdir(x_path(out));
	
	Memfile_SaveBin(&yaz, out);
	
	Memfile_Free(&mem);
	Memfile_Free(&yaz);
	delete(out);
}
