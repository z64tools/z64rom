#include <uLib.h>

/*
   z64ram = 0x80091738
   z64rom = 0xB088D8
 */

u32 func_80091738(PlayState* playState, u8* segment, SkelAnime* skelAnime) {
	return PlayerLib_InitSkelanime(playState, segment, skelAnime);
}