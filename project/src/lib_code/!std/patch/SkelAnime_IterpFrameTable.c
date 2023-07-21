/*
   z64ram = 0x800A2E70
   z64rom = 0xB1A010
 */

#include <uLib.h>

void SkelAnime_InterpFrameTable(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight) {
    SkelAnime_InterpFrameTable_Quat(limbCount, dst, start, target, weight);
}
