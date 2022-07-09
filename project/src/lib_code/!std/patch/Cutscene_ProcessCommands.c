#include <uLib.h>

/*
   z64ram = 0x8006824C
   z64rom = 0xADF3EC
   z64next = 0x80068C3C
 */

void Cutscene_ProcessCommands(PlayState* play, CutsceneContext* csCtx, u8* cutscenePtr) {
	Cutscene_ProcessCmds(play, csCtx, cutscenePtr);
}