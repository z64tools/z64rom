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

_Thread_local MemFile sMemfile;
_Thread_local MemFile sYazfile;

void Yaz_EncodeThread(const char* file) {
	#define THREAD_NUM 16
	
	if (StrEndCase(file, ".zscene")) {
		ItemList list;
		Thread thread[THREAD_NUM];
		
		ItemList_SetFilter(
			&list,
			FILTER_END,
			".toml",
			FILTER_END,
			".zscene",
			FILTER_END,
			".bin",
			FILTER_END,
			".cfg",
			FILTER_END,
			".png",
			FILTER_START,
			"."
		);
		ItemList_List(&list, Path(file), 0, LIST_FILES);
		
		s32 i = 0;
		while (i < list.num) {
			u32 target = Clamp(list.num - i, 0, THREAD_NUM);
			
			for (s32 j = 0; j < target; j++)
				ThreadLock_Create(&thread[j], Yaz_EncodeThread, list.item[i + j]);
			
			for (s32 j = 0; j < target; j++)
				ThreadLock_Join(&thread[j]);
			
			i += THREAD_NUM;
		}
		
		ItemList_Free(&list);
		
		return;
	}
	char* out = HeapMalloc(strlen(file) + strlen("rom/yaz-cache/") + strlen(".yaz") + 1);
	
	strcpy(out, file);
	StrRep(out, "rom/", "rom/yaz-cache/");
	strcat(out, ".yaz");
	
	if (Sys_Stat(out) > Sys_Stat(file))
		return;
	
	MemFile_Malloc(&sMemfile, MbToBin(4.0));
	MemFile_Malloc(&sYazfile, MbToBin(4.0));
	
	if (MemFile_LoadFile(&sMemfile, file))
		printf_error("Could not open [%s]", file);
	
	sYazfile.dataSize = Yaz_Encode(sYazfile.data, sMemfile.data, sMemfile.dataSize);
	
	Sys_MakeDir(Path(out));
	if (MemFile_SaveFile(&sYazfile, out))
		printf_error("Could not save [%s]", out);
	
	MemFile_Free(&sMemfile);
	MemFile_Free(&sYazfile);
#undef THREAD_NUM
}