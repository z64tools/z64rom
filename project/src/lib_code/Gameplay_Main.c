#include <uLib.h>
#include <vt.h>

/*
   z64ram = 0x800BFAE4
   z64rom = 0xB36C84
 */

void Gameplay_Main(GameState* thisx) {
	PlayState* playState = (PlayState*)thisx;
	NewPlay_Main(playState);
}