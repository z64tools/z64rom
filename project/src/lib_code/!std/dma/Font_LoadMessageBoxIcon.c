#include <uLib.h>

/*
   z64ram = 0x8006EEBC
   z64rom = 0xAE605C
 */

void Font_LoadMessageBoxIcon(Font* font, u16 icon) {
    DmaMgr_SendRequest1(
        font->iconBuf,
        gDmaDataTable[18].vromStart + 4 * 0x1000 + icon * FONT_CHAR_TEX_SIZE,
        FONT_CHAR_TEX_SIZE,
        "",
        0
    );
}
