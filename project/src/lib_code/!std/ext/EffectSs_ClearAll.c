#include <uLib.h>
#include "code/z_effect_soft_sprite.h"

/*
   z64ram = 0x80027410
   z64rom = 0xA9E5B0
   z64next = 0x800274E0
 */

void EffectSs_ClearAll(PlayState* playState) {
    u32 i;
    EffectSsOverlay* overlay;
    void* addr;
    
    sEffectSsInfo.table = NULL;
    sEffectSsInfo.searchStartIndex = 0;
    sEffectSsInfo.tableSize = 0;
    
#if 0
    // This code doesn 't actually work, since table was just set to NULL and tableSize to 0
    for (effectSs = &sEffectSsInfo.table[0]; effectSs < &sEffectSsInfo.table[sEffectSsInfo.tableSize]; effectSs++) {
        EffectSs_Delete(effectSs);
    }
#endif
    
    overlay = &gEffectSsOverlayTable[0];
    for (i = 0; i < ARRAY_COUNT(gEffectSsOverlayTable); i++) {
        addr = overlay->loadedRamAddr;
        
        if (addr != NULL)
            ZeldaArena_FreeDebug(addr, "../z_effect_soft_sprite.c", 337);
        
        overlay->loadedRamAddr = NULL;
        overlay++;
    }
}
