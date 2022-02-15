#include "z64rom.h"
#include "texture/n64texconv.h"

#ifndef __IDE_FLAG__
	#define STB_IMAGE_IMPLEMENTATION
	#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "texture/stb_image.h"
#include "texture/stb_image_write.h"

char* sTexelFormat[] = {
	"rgba",
	"yuv",
	"ci",
	"ia",
	"i",
};

char* sTexelBitSize[] = {
	"4",
	"8",
	"16",
	"32",
};

char* sTexelAlphaMethod[] = {
	"edge",
	"alpha",
	"white",
	"black",
	"user"
};

void Texel_SaveToPNG(
	RomFile* romFile,
	const char* filename,
	TexelFormat fmt,
	TexelBitSize bs,
	s32 w, s32 h
) {
	MemFile texel = MemFile_Initialize();
	
	MemFile_Malloc(&texel, romFile->size * 16);
	
	n64texconv_to_rgba8888(
		texel.data,
		romFile->data,
		NULL,
		(s32)fmt,
		(s32)bs,
		w,
		h
	);
	stbi_write_png(filename, w, h, 4, texel.data, 0);
	MemFile_Free(&texel);
}