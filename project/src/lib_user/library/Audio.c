#include <uLib.h>

void Audio_PlaySys(u16 flag) {
	Audio_PlaySoundGeneral(flag, &D_801333D4, 4, &D_801333E0, &D_801333E0, &D_801333E8);
}