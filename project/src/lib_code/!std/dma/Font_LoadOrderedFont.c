#include <uLib.h>

/*
   z64ram = 0x8006EF10
   z64rom = 0xAE60B0
 */

void Font_LoadOrderedFont(Font* font) {
    u32 loadOffset;
    s32 codePointIndex = 0;
    u8* writeLocation;
    
    while (1) {
        writeLocation = &font->fontBuf[codePointIndex * FONT_CHAR_TEX_SIZE];
        loadOffset = gFontOrdering[codePointIndex] * FONT_CHAR_TEX_SIZE;
        if (gFontOrdering[codePointIndex] == 0) {
            loadOffset = 0;
        }
        
        DmaMgr_SendRequest1(
            writeLocation,
            gDmaDataTable[20].vromStart + loadOffset,
            FONT_CHAR_TEX_SIZE,
            "",
            0
        );
        if (gFontOrdering[codePointIndex] == 0x8C) {
            break;
        }
        codePointIndex++;
    }
}
