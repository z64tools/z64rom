#include <uLib.h>

/*
   z64ram = 0x800272B0
   z64rom = 0xA9E450
 */

void EffectSs_InitInfo(GlobalContext* globalCtx, s32 tableSize) {
	u32 i;
	EffectSs* effectSs;
	EffectSsOverlay* overlay;
	
	sEffectSsInfo.table = GameState_Alloc(&globalCtx->state, tableSize * sizeof(EffectSs), __FUNCTION__, __LINE__);
	Assert(sEffectSsInfo.table != NULL);
	
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