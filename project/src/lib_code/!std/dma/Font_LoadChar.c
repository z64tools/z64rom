#include <uLib.h>

/*
   z64ram = 0x8006EE60
   z64rom = 0xAE6000
 */

void _Font_LoadChar(Font* font, u8 character, u16 codePointIndex) {
    DmaMgr_SendRequest1(
        &font->charTexBuf[codePointIndex],
        gDmaDataTable[20].vromStart + character * FONT_CHAR_TEX_SIZE,
        FONT_CHAR_TEX_SIZE,
        "",
        0
    );
}
