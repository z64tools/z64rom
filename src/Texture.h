#include "ExtLib.h"

typedef enum {
	TEX_FMT_RGBA,
	TEX_FMT_YUV,
	TEX_FMT_CI,
	TEX_FMT_IA,
	TEX_FMT_I,
	TEX_FMT_1BIT,
} TexelFormat;

typedef enum {
	TEX_BSIZE_4,
	TEX_BSIZE_8,
	TEX_BSIZE_16,
	TEX_BSIZE_32
} TexelBitSize;

typedef enum {
	TEX_ALPHA_EDGE,
	TEX_ALPHA_ALPHA,
	TEX_ALPHA_WHITE,
	TEX_ALPHA_BLACK,
	TEX_ALPHA_USER,
} TexelAlphaMethod;

struct RomFile;

void Texel_SaveToPNG(
	struct RomFile* romFile,
	const char* filename,
	TexelFormat fmt,
	TexelBitSize bs,
	s32 w, s32 h
);