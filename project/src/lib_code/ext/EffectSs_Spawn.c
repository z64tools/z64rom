#include <ULib.h>

/*
   z64ram = 0x80027798
   z64rom = 0xA9E938
 */

void EffectSs_Spawn(GlobalContext* globalCtx, s32 type, s32 priority, void* initParams) {
	uLib_EffectSs_Spawn(globalCtx, type, priority, initParams);
}