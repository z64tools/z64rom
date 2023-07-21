#include <uLib.h>

void Audio_PlaySys(u16 flag) {
    Audio_PlaySfxGeneral(flag, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}
