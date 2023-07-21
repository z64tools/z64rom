#include "z64rom.h"
#include "texture/n64texconv.h"

void Texture_Dump(RomFile* romFile, const char* filename, TexelFormat fmt, TexelBitSize bs, s32 w, s32 h) {
	Image img = Image_New();
	
	Image_Alloc(&img, w, h, 4);
	n64texconv_to_rgba8888(img.data, romFile->data, NULL, (s32)fmt, (s32)bs, w, h);
	Image_Save(&img, filename);
	Image_Free(&img);
}

Image* Texture_Load(const char* filename, TexelFormat fmt, TexelBitSize bs, s32 _x, s32 _y) {
	Image* img = new(Image);
	
	*img = Image_New();
	Image_Load(img, filename);
	
	if ((_x && _y) && (img->x != _x || img->y != _y))
		warn(gLang.patch.warn_texture_res_mismatch, filename, img->x, img->y, _x, _y);
	n64texconv_to_n64(img->data, img->data, NULL, 0, (s32)fmt, (s32)bs, img->x, img->y, &img->size);
	
	return img;
}
