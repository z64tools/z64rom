#include <uLib.h>

/*
   z64ram = 0x80109968
   z64rom = 0xB80B08
 */

#ifndef R_TEXTBOX_ICON_XPOS
#define __OLD_Z64HDR__

#define MESSAGE_STATIC_TEX_SIZE 0x1000

#define R_TEXT_LINE_SPACING        XREG(56)
#define R_TEXT_CHAR_SCALE          XREG(57)
#define R_TEXTBOX_ICON_XPOS        YREG(71)
#define R_TEXTBOX_ICON_YPOS        YREG(72)
#define R_TEXTBOX_ICON_SIZE        YREG(75)
#define R_TEXTBOX_X                VREG(0)
#define R_TEXTBOX_Y                VREG(1)
#define R_TEXTBOX_END_XPOS         XREG(64)
#define R_TEXTBOX_END_YPOS         XREG(65)
#define R_TEXTBOX_WIDTH_TARGET     XREG(74)
#define R_TEXTBOX_HEIGHT_TARGET    XREG(75)
#define R_TEXTBOX_TEXWIDTH_TARGET  XREG(76)
#define R_TEXTBOX_TEXHEIGHT_TARGET XREG(77)
#define R_TEXT_ADJUST_COLOR_1_R    VREG(33)
#define R_TEXT_ADJUST_COLOR_1_G    VREG(34)
#define R_TEXT_ADJUST_COLOR_1_B    VREG(35)
#define R_TEXT_ADJUST_COLOR_2_R    VREG(36)
#define R_TEXT_ADJUST_COLOR_2_G    VREG(37)
#define R_TEXT_ADJUST_COLOR_2_B    VREG(38)
#define R_TEXT_CHOICE_XPOS         XREG(66)
#define R_TEXT_CHOICE_YPOS(choice) XREG(67 + (choice))
#define R_TEXT_INIT_XPOS    XREG(54)
#define R_TEXT_INIT_YPOS    XREG(55)
#define R_TEXTBOX_BG_YPOS   XREG(61)
#define R_TEXTBOX_CLEF_XPOS VREG(7)
#define R_TEXTBOX_CLEF_YPOS VREG(8)
#endif

void Message_LoadItemIcon(PlayState* playState, u16 itemId, s16 y) {
    static s16 sIconItem32XOffsets[] = { 74, 74, 74 };
    static s16 sIconItem24XOffsets[] = { 72, 72, 72 };
    MessageContext* msgCtx = &playState->msgCtx;
    InterfaceContext* interfaceCtx = &playState->interfaceCtx;
    
    if (itemId == ITEM_DUNGEON_MAP) {
        interfaceCtx->mapPalette[30] = 0xFF;
        interfaceCtx->mapPalette[31] = 0xFF;
    }
    if (itemId < ITEM_MEDALLION_FOREST) {
        R_TEXTBOX_ICON_XPOS = R_TEXT_INIT_XPOS - sIconItem32XOffsets[gSaveContext.language];
        R_TEXTBOX_ICON_YPOS = y + 6;
        R_TEXTBOX_ICON_SIZE = 32;
        DmaMgr_SendRequest1(
            (void*)((u32)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE),
            gDmaDataTable[7].vromStart + (itemId * 0x1000),
            0x1000,
            "",
            0
        );
    } else {
        R_TEXTBOX_ICON_XPOS = R_TEXT_INIT_XPOS - sIconItem24XOffsets[gSaveContext.language];
        R_TEXTBOX_ICON_YPOS = y + 10;
        R_TEXTBOX_ICON_SIZE = 24;
        DmaMgr_SendRequest1(
            (void*)((u32)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE),
            gDmaDataTable[8].vromStart + (itemId - ITEM_MEDALLION_FOREST) * 0x900,
            0x900,
            "",
            0
        );
    }
#ifdef __OLD_Z64HDR__
    *((u16*)((u8*)msgCtx + 0xE3CE)) += 1;
    msgCtx->unk_E3E6[0] = 1;
#else
    msgCtx->msgBufPos++;
    msgCtx->choiceNum = 1;
#endif
}
