/*
 * File: z_opening.c
 * Overlay: ovl_opening
 * Description: Initializes the game into the title screen
 */

#include <uLib.h>
#include <asm_macros.h>

Asm_SymbolAlias("__z64_init", TitleSetup_Init);
Asm_SymbolAlias("__z64_dest", Opening_Destroy);

void Opening_SetupTitleScreen(TitleSetupState* this) {
    gExitParam.nextEntranceIndex = 0x0209;
    gSaveContext.gameMode = 1;
    this->state.running = false;
    gSaveContext.linkAge = LINK_AGE_ADULT;
    Sram_InitDebugSave();
    gSaveContext.cutsceneIndex = 0xFFF3;
    gSaveContext.sceneLayer = 7;
    SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);
    
#if DEV_SCENE_INDEX
    gSaveContext.gameMode = 0;
    gSaveContext.entranceIndex = 0x8000;
    gExitParam.nextEntranceIndex = 0x8000;
    gExitParam.exit = (NewExit) {
        .sceneIndex = DEV_SCENE_INDEX,
        .spawnIndex = DEV_SPAWN_INDEX,
        .headerIndex = DEV_HEADER_INDEX,
        .fadeIn = 2,
        .fadeOut = 2,
    };
    gSaveContext.linkAge = DEV_SPAWN_AGE;
    gSaveContext.cutsceneIndex = 0x0;
#endif
}

void Opening_Main(GameState* thisx) {
    TitleSetupState* this = (TitleSetupState*)thisx;
    
    Gfx_SetupFrame(this->state.gfxCtx, 0, 0, 0);
    Opening_SetupTitleScreen(this);
}

void Opening_Destroy(GameState* thisx) {
}

void TitleSetup_Init(GameState* thisx) {
    TitleSetupState* this = (TitleSetupState*)thisx;
    
    R_UPDATE_RATE = 1;
    Matrix_Init(&this->state);
    View_Init(&this->view, this->state.gfxCtx);
    this->state.main = Opening_Main;
    this->state.destroy = Opening_Destroy;
}
