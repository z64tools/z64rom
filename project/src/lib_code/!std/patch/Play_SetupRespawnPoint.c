#include <uLib.h>
#include "code/z_demo.h"

/*
   z64ram = 0x800C0AF4
   z64rom = 0xB37C94
   z64next = 0x800C0B60
 */

void Play_SetupRespawnPoint(PlayState* this, s32 respawnMode, s32 playerParams) {
	NewPlay_SetupRespawn(this, respawnMode, playerParams);
}