#include <uLib.h>

/*
   z64ram = 0x8006EF10
   z64rom = 0xAE60B0
 */

extern const char _message_0xFFFC_nes[];
extern const char _message_0xFFFD_nes[];
asm ("_message_0xFFFC_nes = 0x070380D4");
asm ("_message_0xFFFD_nes = 0x0703811C");

#ifndef CTRL_NEWLINE
#define MESSAGE_NEWLINE 0x01
#define MESSAGE_END     0x02
#endif

void Font_LoadOrderedFont(Font* font) {
	s32 len;
	s32 fontStatic;
	u32 fontBuf;
	s32 codePointIndex;
	s32 fontBufIndex;
	s32 offset;
	
	font->msgOffset = _message_0xFFFC_nes - (const char*)0x07000000;
	len = font->msgLength = _message_0xFFFD_nes - _message_0xFFFC_nes;
	
	DmaMgr_SendRequest1(
		font->msgBuf,
		gDmaDataTable[21].vromStart + font->msgOffset,
		len,
		"",
		0
	);
	
	for (fontBufIndex = 0, codePointIndex = 0; font->msgBuf[codePointIndex] != MESSAGE_END; codePointIndex++) {
		if (codePointIndex > len) {
			return;
		}
		
		if (font->msgBuf[codePointIndex] != MESSAGE_NEWLINE) {
			fontBuf = (u32)font->fontBuf + fontBufIndex * 8;
			fontStatic = gDmaDataTable[20].vromStart;
			
			offset = (font->msgBuf[codePointIndex] - '\x20') * FONT_CHAR_TEX_SIZE;
			DmaMgr_SendRequest1((void*)fontBuf, fontStatic + offset, FONT_CHAR_TEX_SIZE, "", 0);
			fontBufIndex += FONT_CHAR_TEX_SIZE / 8;
		}
	}
}