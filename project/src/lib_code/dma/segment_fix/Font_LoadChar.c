#include <oot_mq_debug/z64hdr.h>

void _Font_LoadChar(Font* font, u8 character, u16 codePointIndex) {
	DmaMgr_SendRequest1(
		&font->charTexBuf[codePointIndex],
		gDmaDataTable[20].vromStart + character * FONT_CHAR_TEX_SIZE,
		FONT_CHAR_TEX_SIZE,
		"",
		0
	);
}
