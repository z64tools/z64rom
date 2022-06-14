#include <uLib.h>

/*
   z64ram = 0x80027798
   z64rom = 0xA9E938
 */

void EffectSs_Spawn(PlayState* playState, s32 type, s32 priority, void* initParams) {
	Overlay_EffectSpawn(playState, type, priority, initParams);
}