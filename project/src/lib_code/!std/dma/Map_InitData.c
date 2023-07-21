#include <uLib.h>

/*
   z64ram = 0x80080AB4
   z64rom = 0xAF7C54
 */

extern s16 sEntranceIconMapIndex;
asm ("sEntranceIconMapIndex = 0x80123A5C");
asm ("gMapData = 0x8015FFD0");

void Map_InitData(PlayState* playState, s16 room) {
    s32 mapIndex = gSaveContext.mapIndex;
    InterfaceContext* interfaceCtx = &playState->interfaceCtx;
    s16 extendedMapIndex;
    
    switch (playState->sceneId) {
        case SCENE_SPOT00:
        case SCENE_SPOT01:
        case SCENE_SPOT02:
        case SCENE_SPOT03:
        case SCENE_SPOT04:
        case SCENE_SPOT05:
        case SCENE_SPOT06:
        case SCENE_SPOT07:
        case SCENE_SPOT08:
        case SCENE_SPOT09:
        case SCENE_SPOT10:
        case SCENE_SPOT11:
        case SCENE_SPOT12:
        case SCENE_SPOT13:
        case SCENE_SPOT15:
        case SCENE_SPOT16:
        case SCENE_SPOT17:
        case SCENE_SPOT18:
        case SCENE_SPOT20:
        case SCENE_GANON_TOU:
            extendedMapIndex = mapIndex;
            if (playState->sceneId == SCENE_SPOT02) {
                if (CHECK_QUEST_ITEM(QUEST_SONG_NOCTURNE)) {
                    extendedMapIndex = 0x14;
                }
            } else if (playState->sceneId == SCENE_SPOT06) {
                if ((LINK_AGE_IN_YEARS == YEARS_ADULT) && !CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER)) {
                    extendedMapIndex = 0x15;
                }
            } else if (playState->sceneId == SCENE_SPOT09) {
                if ((LINK_AGE_IN_YEARS == YEARS_ADULT) && !((gSaveContext.eventChkInf[9] & 0xF) == 0xF)) {
                    extendedMapIndex = 0x16;
                }
            } else if (playState->sceneId == SCENE_SPOT12) {
                if ((gSaveContext.eventChkInf[9] & 0xF) == 0xF) {
                    extendedMapIndex = 0x17;
                }
            }
            
            sEntranceIconMapIndex = extendedMapIndex;
            DmaMgr_SendRequest1(
                interfaceCtx->mapSegment,
                gDmaDataTable[25].vromStart + gMapData->owMinimapTexOffset[extendedMapIndex],
                gMapData->owMinimapTexSize[mapIndex],
                "",
                0
            );
            interfaceCtx->unk_258 = mapIndex;
            break;
        case SCENE_YDAN:
        case SCENE_DDAN:
        case SCENE_BDAN:
        case SCENE_BMORI1:
        case SCENE_HIDAN:
        case SCENE_MIZUSIN:
        case SCENE_JYASINZOU:
        case SCENE_HAKADAN:
        case SCENE_HAKADANCH:
        case SCENE_ICE_DOUKUTO:
        case SCENE_YDAN_BOSS:
        case SCENE_DDAN_BOSS:
        case SCENE_BDAN_BOSS:
        case SCENE_MORIBOSSROOM:
        case SCENE_FIRE_BS:
        case SCENE_MIZUSIN_BS:
        case SCENE_JYASINBOSS:
        case SCENE_HAKADAN_BS:
            DmaMgr_SendRequest1(
                playState->interfaceCtx.mapSegment,
                gDmaDataTable[26].vromStart +
                ((gMapData->dgnMinimapTexIndexOffset[mapIndex] + room) * 0xFF0),
                0xFF0,
                "",
                0
            );
            R_COMPASS_OFFSET_X = gMapData->roomCompassOffsetX[mapIndex][room];
            R_COMPASS_OFFSET_Y = gMapData->roomCompassOffsetY[mapIndex][room];
            Map_SetFloorPalettesData(playState, VREG(30));
            break;
    }
}
