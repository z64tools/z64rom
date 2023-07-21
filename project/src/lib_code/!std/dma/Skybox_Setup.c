#include <uLib.h>

/*
   z64ram = 0x800AF218
   z64rom = 0xB263B8
 */

typedef struct {
    DmaEntry* file;
    DmaEntry* palette;
} NewSkyboxFiles;

s8 gSkyboxDmaIdTable[];

f32 Environment_LerpWeight(u16 max, u16 min, u16 val);
extern NewSkyboxFiles gNewSkyboxFiles[];
extern u8 gWeatherMode;
asm ("gNewSkyboxFiles = 0x8011FD3C");

void Skybox_Setup(PlayState* playState, SkyboxContext* skyboxCtx, s16 skyboxId) {
    u32 size;
    s16 i;
    u8 imageIdx1 = 0;
    u8 imageIdx2 = 0;
    u32 start;
    s32 phi_v1;
    
    osSyncPrintf("Skybox_Setup ");
    
    if (gSkyboxDmaIdTable[skyboxId] > -1) {
        DmaEntry* entry = &gDmaDataTable[gSkyboxDmaIdTable[skyboxId] + 941];
        
        start = entry->vromStart;
        size = entry->vromEnd - entry->vromStart;
        skyboxCtx->staticSegments[0] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
        DmaMgr_SendRequest1(skyboxCtx->staticSegments[0], start, size, "Skybox_Setup", __LINE__);
        entry++;
        start = entry->vromStart;
        size = entry->vromEnd - entry->vromStart;
        skyboxCtx->palettes = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
        DmaMgr_SendRequest1(skyboxCtx->palettes, start, size, "Skybox_Setup", __LINE__);
    }
    
    switch (skyboxId) {
        case SKYBOX_NORMAL_SKY:
            phi_v1 = 0;
            if (
                gSaveContext.retainWeatherMode != 0 && gSaveContext.sceneLayer < 4 && gWeatherMode > 0 &&
                gWeatherMode < 6
            ) {
                phi_v1 = 1;
            }
            
            for (i = 0; i < 9; i++) {
                if (
                    gSaveContext.skyboxTime >= gTimeBasedSkyboxConfigs[phi_v1][i].startTime &&
                    (gSaveContext.skyboxTime < gTimeBasedSkyboxConfigs[phi_v1][i].endTime ||
                    gTimeBasedSkyboxConfigs[phi_v1][i].endTime == 0xFFFF)
                ) {
                    playState->envCtx.skybox1Index = imageIdx1 = gTimeBasedSkyboxConfigs[phi_v1][i].skybox1Index;
                    playState->envCtx.skybox2Index = imageIdx2 = gTimeBasedSkyboxConfigs[phi_v1][i].skybox2Index;
                    if (gTimeBasedSkyboxConfigs[phi_v1][i].changeSkybox != 0) {
                        playState->envCtx.skyboxBlend =
                            Environment_LerpWeight(
                            gTimeBasedSkyboxConfigs[phi_v1][i].endTime,
                            gTimeBasedSkyboxConfigs[phi_v1][i].startTime,
                            ((void)0, gSaveContext.skyboxTime)
                            ) *
                            255.0f;
                    } else {
                        playState->envCtx.skyboxBlend = 0;
                    }
                    break;
                }
            }
            
            size = gNewSkyboxFiles[imageIdx1].file->vromEnd - gNewSkyboxFiles[imageIdx1].file->vromStart;
            skyboxCtx->staticSegments[0] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
            
            DmaMgr_SendRequest1(
                skyboxCtx->staticSegments[0],
                gNewSkyboxFiles[imageIdx1].file->vromStart,
                size,
                "Skybox_Setup",
                __LINE__
            );
            
            size = gNewSkyboxFiles[imageIdx2].file->vromEnd - gNewSkyboxFiles[imageIdx2].file->vromStart;
            skyboxCtx->staticSegments[1] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
            
            DmaMgr_SendRequest1(
                skyboxCtx->staticSegments[1],
                gNewSkyboxFiles[imageIdx2].file->vromStart,
                size,
                "Skybox_Setup",
                __LINE__
            );
            
            if ((imageIdx1 & 1) ^ ((imageIdx1 & 4) >> 2)) {
                size = gNewSkyboxFiles[imageIdx1].palette->vromEnd - gNewSkyboxFiles[imageIdx1].palette->vromStart;
                
                skyboxCtx->palettes = GameState_Alloc(&playState->state, size * 2, "Skybox_Setup", __LINE__);
                
                DmaMgr_SendRequest1(
                    skyboxCtx->palettes,
                    gNewSkyboxFiles[imageIdx1].palette->vromStart,
                    size,
                    "Skybox_Setup",
                    __LINE__
                );
                DmaMgr_SendRequest1(
                    (void*)((u32)skyboxCtx->palettes + size),
                    gNewSkyboxFiles[imageIdx2].palette->vromStart,
                    size,
                    "Skybox_Setup",
                    __LINE__
                );
            } else {
                size = gNewSkyboxFiles[imageIdx1].palette->vromEnd - gNewSkyboxFiles[imageIdx1].palette->vromStart;
                
                skyboxCtx->palettes = GameState_Alloc(&playState->state, size * 2, "Skybox_Setup", __LINE__);
                
                DmaMgr_SendRequest1(
                    skyboxCtx->palettes,
                    gNewSkyboxFiles[imageIdx2].palette->vromStart,
                    size,
                    "Skybox_Setup",
                    __LINE__
                );
                DmaMgr_SendRequest1(
                    (void*)((u32)skyboxCtx->palettes + size),
                    gNewSkyboxFiles[imageIdx1].palette->vromStart,
                    size,
                    "Skybox_Setup",
                    __LINE__
                );
            }
            break;
            
        case SKYBOX_OVERCAST_SUNSET:
            start = gDmaDataTable[953].vromStart;
            size = gDmaDataTable[953].vromEnd - start;
            
            skyboxCtx->staticSegments[0] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
            DmaMgr_SendRequest1(skyboxCtx->staticSegments[0], start, size, "Skybox_Setup", 1159);
            skyboxCtx->staticSegments[1] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
            DmaMgr_SendRequest1(skyboxCtx->staticSegments[1], start, size, "Skybox_Setup", 1166);
            
            start = gDmaDataTable[953 + 1].vromStart;
            size = gDmaDataTable[953 + 1].vromEnd - start;
            skyboxCtx->palettes = GameState_Alloc(&playState->state, size * 2, "Skybox_Setup", __LINE__);
            
            DmaMgr_SendRequest1(skyboxCtx->palettes, start, size, "Skybox_Setup", __LINE__);
            DmaMgr_SendRequest1((void*)((u32)skyboxCtx->palettes + size), start, size, "Skybox_Setup", __LINE__);
            break;
            
        case SKYBOX_CUTSCENE_MAP:
            start = gDmaDataTable[957].vromStart;
            size = gDmaDataTable[957].vromEnd - start;
            skyboxCtx->staticSegments[0] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
            
            DmaMgr_SendRequest1(skyboxCtx->staticSegments[0], start, size, "Skybox_Setup", __LINE__);
            
            start = gDmaDataTable[957 + 2].vromStart;
            size = gDmaDataTable[957 + 2].vromEnd - start;
            skyboxCtx->staticSegments[1] = GameState_Alloc(&playState->state, size, "Skybox_Setup", __LINE__);
            
            DmaMgr_SendRequest1(skyboxCtx->staticSegments[1], start, size, "Skybox_Setup", __LINE__);
            
            start = gDmaDataTable[958].vromStart;
            size = gDmaDataTable[958].vromEnd - start;
            skyboxCtx->palettes = GameState_Alloc(&playState->state, size * 2, "Skybox_Setup", __LINE__);
            
            DmaMgr_SendRequest1(skyboxCtx->palettes, start, size, "Skybox_Setup", __LINE__);
            DmaMgr_SendRequest1(
                (void*)((u32)skyboxCtx->palettes + size),
                gDmaDataTable[958 + 2].vromStart,
                size,
                "Skybox_Setup",
                __LINE__
            );
            break;
            
        case SKYBOX_MARKET_ADULT:
        case SKYBOX_HOUSE_LINK:
        case SKYBOX_MARKET_CHILD_DAY:
        case SKYBOX_MARKET_CHILD_NIGHT:
        case SKYBOX_HOUSE_KNOW_IT_ALL_BROTHERS:
        case SKYBOX_STABLES:
        case SKYBOX_HOUSE_KAKARIKO:
        case SKYBOX_HOUSE_RICHARD:
        case SKYBOX_HOUSE_IMPA:
            skyboxCtx->unk_140 = 1;
            break;
            
        case SKYBOX_BAZAAR:
        case SKYBOX_HAPPY_MASK_SHOP:
        case SKYBOX_KOKIRI_SHOP:
        case SKYBOX_GORON_SHOP:
        case SKYBOX_ZORA_SHOP:
        case SKYBOX_POTION_SHOP_KAKARIKO:
        case SKYBOX_POTION_SHOP_MARKET:
        case SKYBOX_BOMBCHU_SHOP:
            skyboxCtx->unk_140 = 1;
            skyboxCtx->rot.y = 0.8f;
            break;
            
        case SKYBOX_TENT:
        case SKYBOX_HOUSE_OF_TWINS:
        case SKYBOX_HOUSE_MIDO:
        case SKYBOX_HOUSE_SARIA:
        case SKYBOX_HOUSE_ALLEY:
            skyboxCtx->unk_140 = 2;
            break;
    }
    
    osSyncPrintf("OK\n");
}

#define ID(x) ((x) - 941)

s8 gSkyboxDmaIdTable[] = {
    /* SKYBOX_NONE                       */ -1,
    /* SKYBOX_NORMAL_SKY                 */ -1,
    /* SKYBOX_BAZAAR                     */ ID(977),
    /* SKYBOX_OVERCAST_SUNSET            */ -1,
    /* SKYBOX_MARKET_ADULT               */ ID(965),
    /* SKYBOX_CUTSCENE_MAP               */ -1,
    -1,
    /* SKYBOX_HOUSE_LINK                 */ ID(967),
    -1,
    /* SKYBOX_MARKET_CHILD_DAY           */ ID(961),
    /* SKYBOX_MARKET_CHILD_NIGHT         */ ID(963),
    /* SKYBOX_HAPPY_MASK_SHOP            */ ID(1003),
    /* SKYBOX_HOUSE_KNOW_IT_ALL_BROTHERS */ ID(969),
    -1,
    /* SKYBOX_HOUSE_OF_TWINS             */ ID(971),
    /* SKYBOX_STABLES                    */ ID(979),
    /* SKYBOX_HOUSE_KAKARIKO             */ ID(981),
    /* SKYBOX_KOKIRI_SHOP                */ ID(987),
    -1,
    /* SKYBOX_GORON_SHOP                 */ ID(989),
    /* SKYBOX_ZORA_SHOP                  */ ID(991),
    -1,
    /* SKYBOX_POTION_SHOP_KAKARIKO       */ ID(993),
    /* SKYBOX_POTION_SHOP_MARKET         */ ID(995),
    /* SKYBOX_BOMBCHU_SHOP               */ ID(997),
    -1,
    /* SKYBOX_HOUSE_RICHARD              */ ID(985),
    /* SKYBOX_HOUSE_IMPA                 */ ID(999),
    /* SKYBOX_TENT                       */ ID(1001),
    /* SKYBOX_UNSET_1D                   */ -1,
    -1,
    -1,
    /* SKYBOX_HOUSE_MIDO                 */ ID(973),
    /* SKYBOX_HOUSE_SARIA                */ ID(975),
    /* SKYBOX_HOUSE_ALLEY                */ ID(983),
    -1,
    -1,
    -1,
    -1,
    /* SKYBOX_UNSET_27                   */ -1,
};