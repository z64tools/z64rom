#include <uLib.h>

/*
   z64ram = 0x801109B0
   z64rom = 0xB87B50
 */

void func_801109B0(PlayState* playState) {
    InterfaceContext* interfaceCtx = &playState->interfaceCtx;
    u32 parameterSize;
    u16 doActionOffset;
    u8 temp;
    
    gSaveContext.sunsSongState = SUNSSONG_INACTIVE;
    gSaveContext.unk_13E8 = gSaveContext.unk_13EA = 0;
    
    View_Init(&interfaceCtx->view, playState->state.gfxCtx);
    
    interfaceCtx->unk_1FA = interfaceCtx->unk_261 = interfaceCtx->unk_1FC = 0;
    interfaceCtx->unk_1EC = interfaceCtx->unk_1EE = interfaceCtx->unk_1F0 = 0;
    interfaceCtx->unk_22E = 0;
    interfaceCtx->lensMagicConsumptionTimer = 16;
    interfaceCtx->unk_1F4 = 0.0f;
    interfaceCtx->unk_228 = XREG(95);
    interfaceCtx->minimapAlpha = 0;
    interfaceCtx->unk_260 = 0;
    interfaceCtx->unk_244 = interfaceCtx->aAlpha = interfaceCtx->bAlpha = interfaceCtx->cLeftAlpha =
        interfaceCtx->cDownAlpha = interfaceCtx->cRightAlpha = interfaceCtx->healthAlpha = interfaceCtx->startAlpha =
        interfaceCtx->magicAlpha = 0;
    
    parameterSize = gDmaDataTable[940].vromEnd - gDmaDataTable[940].vromStart;
    
    interfaceCtx->parameterSegment = GameState_Alloc(&playState->state, parameterSize, "", __LINE__);
    DmaMgr_SendRequest1(
        interfaceCtx->parameterSegment,
        gDmaDataTable[940].vromStart,
        parameterSize,
        "",
        __LINE__
    );
    
    interfaceCtx->doActionSegment = GameState_Alloc(&playState->state, 0x480, "", __LINE__);
    
    if (gSaveContext.language == LANGUAGE_ENG) {
        doActionOffset = 0;
    } else if (gSaveContext.language == LANGUAGE_GER) {
        doActionOffset = 0x2B80;
    } else {
        doActionOffset = 0x5700;
    }
    
    DmaMgr_SendRequest1(
        interfaceCtx->doActionSegment,
        gDmaDataTable[17].vromStart + doActionOffset,
        0x300,
        "",
        __LINE__
    );
    
    if (gSaveContext.language == LANGUAGE_ENG) {
        doActionOffset = 0x480;
    } else if (gSaveContext.language == LANGUAGE_GER) {
        doActionOffset = 0x3000;
    } else {
        doActionOffset = 0x5B80;
    }
    
    DmaMgr_SendRequest1(
        interfaceCtx->doActionSegment + 0x300,
        gDmaDataTable[17].vromStart + doActionOffset,
        0x180,
        "",
        __LINE__
    );
    
    interfaceCtx->iconItemSegment = GameState_Alloc(&playState->state, 0x4000, "", __LINE__);
    
    if (gSaveContext.equips.buttonItems[0] < 0xF0) {
        DmaMgr_SendRequest1(
            interfaceCtx->iconItemSegment,
            gDmaDataTable[7].vromStart + gSaveContext.equips.buttonItems[0] * 0x1000,
            0x1000,
            "",
            __LINE__
        );
    } else if (gSaveContext.equips.buttonItems[0] != 0xFF) {
        DmaMgr_SendRequest1(
            interfaceCtx->iconItemSegment,
            gDmaDataTable[7].vromStart + gSaveContext.equips.buttonItems[0] * 0x1000,
            0x1000,
            "",
            __LINE__
        );
    }
    
    if (gSaveContext.equips.buttonItems[1] < 0xF0) {
        DmaMgr_SendRequest1(
            interfaceCtx->iconItemSegment + 0x1000,
            gDmaDataTable[7].vromStart + gSaveContext.equips.buttonItems[1] * 0x1000,
            0x1000,
            "",
            __LINE__
        );
    }
    
    if (gSaveContext.equips.buttonItems[2] < 0xF0) {
        DmaMgr_SendRequest1(
            interfaceCtx->iconItemSegment + 0x2000,
            gDmaDataTable[7].vromStart + gSaveContext.equips.buttonItems[2] * 0x1000,
            0x1000,
            "",
            __LINE__
        );
    }
    
    if (gSaveContext.equips.buttonItems[3] < 0xF0) {
        DmaMgr_SendRequest1(
            interfaceCtx->iconItemSegment + 0x3000,
            gDmaDataTable[7].vromStart + gSaveContext.equips.buttonItems[3] * 0x1000,
            0x1000,
            "",
            __LINE__
        );
    }
    
    if (
        (gSaveContext.timer1State == 4) || (gSaveContext.timer1State == 8) || (gSaveContext.timer2State == 4) ||
        (gSaveContext.timer2State == 10)
    ) {
        if ((gSaveContext.respawnFlag == -1) || (gSaveContext.respawnFlag == 1)) {
            if (gSaveContext.timer1State == 4) {
                gSaveContext.timer1State = 1;
                gSaveContext.timerX[0] = 140;
                gSaveContext.timerY[0] = 80;
            }
        }
        
        if ((gSaveContext.timer1State == 4) || (gSaveContext.timer1State == 8)) {
            temp = 0;
        } else {
            temp = 1;
        }
        
        gSaveContext.timerX[temp] = 26;
        
        if (gSaveContext.healthCapacity > 0xA0) {
            gSaveContext.timerY[temp] = 54;
        } else {
            gSaveContext.timerY[temp] = 46;
        }
    }
    
    if ((gSaveContext.timer1State >= 11) && (gSaveContext.timer1State < 16)) {
        gSaveContext.timer1State = 0;
    }
    
    Health_InitMeter(playState);
    Map_Init(playState);
    
    interfaceCtx->unk_23C = interfaceCtx->unk_242 = 0;
    
    R_ITEM_BTN_X(0) = B_BUTTON_X;
    R_B_BTN_COLOR(0) = 0;
    R_B_BTN_COLOR(1) = 150;
    R_B_BTN_COLOR(2) = 0;
    R_ITEM_ICON_X(0) = B_BUTTON_X;
    R_ITEM_AMMO_X(0) = B_BUTTON_X + 2;
    R_A_BTN_X = A_BUTTON_X;
    R_A_ICON_X = A_BUTTON_X;
    R_A_BTN_COLOR(0) = 80;
    R_A_BTN_COLOR(1) = 80;
    R_A_BTN_COLOR(2) = 225;
}
