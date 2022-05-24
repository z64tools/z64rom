#include <uLib.h>

/*
   z64ram = 0x80027798
   z64rom = 0xA9E938
 */

void EffectSs_Spawn(GlobalContext* globalCtx, s32 type, s32 priority, void* initParams) {
	Overlay_EffectSpawn(globalCtx, type, priority, initParams);
}