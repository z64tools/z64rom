#ifndef Z64ROM_INLINE_MATH_H
#define Z64ROM_INLINE_MATH_H

#include <uLib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

f32 fminf(f32, f32);
f32 fmaxf(f32, f32);
f32 rintf(f32);

static f32 Lerp(f32 x, f32 min, f32 max) {
    return min + (max - min) * x;
}

static f32 Normalize(f32 v, f32 min, f32 max) {
    return (v - min) / (max - min);
}

static f32 Remap(f32 v, f32 iMin, f32 iMax, f32 oMin, f32 oMax) {
    return Lerp(Normalize(v, iMin, iMax), oMin, oMax);
}

#pragma GCC diagnostic pop

#endif