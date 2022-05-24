/*
   z64ram = 0x800BCA64
   z64rom = 0xB33C04
 */

#include <uLib.h>

void __Gameplay_Init(GameState* thisx) {
	Play_Init((void*)thisx);
}