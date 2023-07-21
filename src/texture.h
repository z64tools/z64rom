#include <ext_texel.h>

typedef enum {
	TEX_FMT_RGBA = 0,
	TEX_FMT_YUV,
	TEX_FMT_CI,
	TEX_FMT_IA,
	TEX_FMT_I,
	TEX_FMT_1BIT,
} TexelFormat;

typedef enum {
	TEX_BSIZE_4 = 0,
	TEX_BSIZE_8,
	TEX_BSIZE_16,
	TEX_BSIZE_32
} TexelBitSize;

typedef enum {
	TEX_ALPHA_EDGE = 0,
	TEX_ALPHA_ALPHA,
	TEX_ALPHA_WHITE,
	TEX_ALPHA_BLACK,
	TEX_ALPHA_USER,
} TexelAlphaMethod;

struct RomFile;

void Texture_Dump(
	struct RomFile* romFile,
	const char* filename,
	TexelFormat fmt,
	TexelBitSize bs,
	s32 w, s32 h
);

Image* Texture_Load(
	const char* filename,
	TexelFormat fmt,
	TexelBitSize bs,
	s32 x, s32 y
);