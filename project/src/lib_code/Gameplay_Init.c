/*
   z64ram = 0x800BCA64
   z64rom = 0xB33C04
 */

#include <uLib.h>

void __Gameplay_Init(GameState* thisx) {
	NewPlay_Init((void*)thisx);
}