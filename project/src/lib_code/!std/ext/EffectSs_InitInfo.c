#include <uLib.h>
#include "code/z_effect_soft_sprite.h"

/*
   z64ram = 0x800272B0
   z64rom = 0xA9E450
   z64next = 0x80027410
 */

void EffectSs_InitInfo(PlayState* playState, s32 tableSize) {
    u32 i;
    EffectSs* effectSs;
    EffectSsOverlay* overlay;
    
    sEffectSsInfo.table = GameState_Alloc(&playState->state, tableSize * sizeof(EffectSs), (char*)__FUNCTION__, __LINE__);
    if (sEffectSsInfo.table == NULL)
        __assert("sEffectSsInfo.table != NULL", __FUNCTION__, __LINE__);
    
    sEffectSsInfo.searchStartIndex = 0;
    sEffectSsInfo.tableSize = tableSize;
    
    for (effectSs = &sEffectSsInfo.table[0]; effectSs < &sEffectSsInfo.table[sEffectSsInfo.tableSize]; effectSs++)
        EffectSs_Reset(effectSs);
    
    overlay = &gEffectSsOverlayTable[0];
    for (i = 0; i < ARRAY_COUNT(gEffectSsOverlayTable); i++) {
        overlay->loadedRamAddr = NULL;
        overlay++;
    }
}
