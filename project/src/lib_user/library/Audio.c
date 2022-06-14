#include <uLib.h>

void Audio_PlaySys(u16 flag) {
	Audio_PlaySoundGeneral(flag, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}