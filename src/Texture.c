#include "z64rom.h"
#include "texture/n64texconv.h"

#ifndef __IDE_FLAG__
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "texture/stb_image.h"
#include "texture/stb_image_write.h"

void Texel_Dump(
	RomFile* romFile,
	const char* filename,
	TexelFormat fmt,
	TexelBitSize bs,
	s32 w, s32 h
) {
	MemFile texel = MemFile_Initialize();
	
	MemFile_Alloc(&texel, romFile->size * 128);
	
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

void Texel_Import(
	MemFile* mem,
	const char* filename,
	TexelFormat fmt,
	TexelBitSize bs,
	s32 w, s32 h
) {
	const char* err;
	s32 x, y, c;
	void* png;
	
	MemFile_Reset(mem);
	MemFile_LoadFile(mem, filename);
	
	png = stbi_load_from_memory(mem->data, mem->size, &x, &y, &c, 0);
	
	if (x != w || y != h) {
		printf_error("Image [%s] resolution [%d/%d] does not match expected [%d/%d]", filename, x, y, w, h);
	}
	
	err = n64texconv_to_n64(mem->data, png, NULL, 0, (s32)fmt, (s32)bs, w, h, &mem->size);
	if (err) printf_error("n64texconv: [%s]", err);
	Free(png);
}