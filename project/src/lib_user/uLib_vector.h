#ifndef Z64ROM_INLINE_VECTOR_H
#define Z64ROM_INLINE_VECTOR_H

#include <uLib.h>

#ifndef IsZero
#define IsZero(v) (fabsf(v) < 0.0001)
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static inline Vec3f Vec3f_Cross(Vec3f a, Vec3f b) {
    return (Vec3f) {
               (a.y * b.z - b.y * a.z),
               (a.z * b.x - b.z * a.x),
               (a.x * b.y - b.x * a.y)
    };
}

static inline Vec3s Vec3s_Cross(Vec3s a, Vec3s b) {
    return (Vec3s) {
               (a.y * b.z - b.y * a.z),
               (a.z * b.x - b.z * a.x),
               (a.x * b.y - b.x * a.y)
    };
}

static inline f32 Vec3f_DistXZ(Vec3f a, Vec3f b) {
    f32 dx = b.x - a.x;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline f32 Vec3f_DistXYZ(Vec3f a, Vec3f b) {
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

static inline f32 Vec3s_DistXZ(Vec3s a, Vec3s b) {
    f32 dx = b.x - a.x;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline f32 Vec3s_DistXYZ(Vec3s a, Vec3s b) {
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

static inline f32 Vec2f_DistXZ(Vec2f a, Vec2f b) {
    f32 dx = b.x - a.x;
    f32 dz = b.y - a.y;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline Vec2f Vec2f_Sub(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x - b.x, a.y - b.y };
}

static inline Vec3f Vec3f_Sub(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec2f Vec2f_SubVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x - val, a.y - val };
}

static inline Vec3f Vec3f_SubVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x - val, a.y - val, a.z - val };
}

static inline Vec3s Vec3s_Sub(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec3s Vec3s_SubVal(Vec3s a, s16 val) {
    return (Vec3s) { a.x - val, a.y - val, a.z - val };
}

static inline Vec2f Vec2f_Add(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x + b.x, a.y + b.y };
}

static inline Vec3f Vec3f_Add(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec2f Vec2f_AddVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x + val, a.y + val };
}

static inline Vec3f Vec3f_AddVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x + val, a.y + val, a.z + val };
}

static inline Vec3s Vec3s_Add(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec3s Vec3s_AddVal(Vec3s a, s16 val) {
    return (Vec3s) { a.x + val, a.y + val, a.z + val };
}

static inline Vec2f Vec2f_Div(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x / b.x, a.y / b.y };
}

static inline Vec3f Vec3f_Div(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x / b.x, a.y / b.y, a.z / b.z };
}

static inline Vec2f Vec2f_DivVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x / val, a.y / val };
}

static inline Vec3f Vec3f_DivVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x / val, a.y / val, a.z / val };
}

static inline Vec3s Vec3s_Div(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x / b.x, a.y / b.y, a.z / b.z };
}

static inline Vec3s Vec3s_DivVal(Vec3s a, f32 val) {
    return (Vec3s) { a.x / val, a.y / val, a.z / val };
}

static inline Vec2f Vec2f_Mul(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x* b.x, a.y* b.y };
}

static inline Vec3f Vec3f_Mul(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x* b.x, a.y* b.y, a.z* b.z };
}

static inline Vec2f Vec2f_MulVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x* val, a.y* val };
}

static inline Vec3f Vec3f_MulVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x* val, a.y* val, a.z* val };
}

static inline Vec3s Vec3s_Mul(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x* b.x, a.y* b.y, a.z* b.z };
}

static inline Vec3s Vec3s_MulVal(Vec3s a, f32 val) {
    return (Vec3s) { a.x* val, a.y* val, a.z* val };
}

static inline Vec2f Vec2f_New(f32 x, f32 y) {
    return (Vec2f) { x, y };
}

static inline Vec3f Vec3f_New(f32 x, f32 y, f32 z) {
    return (Vec3f) { x, y, z };
}

static inline Vec3s Vec3s_New(s16 x, s16 y, s16 z) {
    return (Vec3s) { x, y, z };
}

static inline f32 Vec2f_Dot(Vec2f a, Vec2f b) {
    return a.x * b.x + a.y * b.y;
}

static inline f32 Vec3f_Dot(Vec3f a, Vec3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline f32 Vec3s_Dot(Vec3s a, Vec3s b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline f32 Vec2f_MagnitudeSQ(Vec2f a) {
    return Vec2f_Dot(a, a);
}

static inline f32 Vec3f_MagnitudeSQ(Vec3f a) {
    return Vec3f_Dot(a, a);
}

static inline f32 Vec3s_MagnitudeSQ(Vec3s a) {
    return Vec3s_Dot(a, a);
}

static inline f32 Vec2f_Magnitude(Vec2f a) {
    return sqrtf(Vec2f_MagnitudeSQ(a));
}

static inline f32 Vec3f_Magnitude(Vec3f a) {
    return sqrtf(Vec3f_MagnitudeSQ(a));
}

static inline f32 Vec3s_Magnitude(Vec3s a) {
    return sqrtf(Vec3s_MagnitudeSQ(a));
}

static inline Vec2f Vec2f_Normalize(Vec2f a) {
    f32 mgn = Vec2f_Magnitude(a);
    
    if (mgn == 0.0f)
        return Vec2f_MulVal(a, 0.0f);
    else
        return Vec2f_DivVal(a, mgn);
}

static inline Vec3f Vec3f_Normalize(Vec3f a) {
    f32 mgn = Vec3f_Magnitude(a);
    
    if (mgn == 0.0f)
        return Vec3f_MulVal(a, 0.0f);
    else
        return Vec3f_DivVal(a, mgn);
}

static inline Vec3s Vec3s_Normalize(Vec3s a) {
    f32 mgn = Vec3s_Magnitude(a);
    
    if (mgn == 0.0f)
        return Vec3s_MulVal(a, 0.0f);
    else
        return Vec3s_DivVal(a, mgn);
}

static inline Vec2f Vec2f_LineSegDir(Vec2f a, Vec2f b) {
    return Vec2f_Normalize(Vec2f_Sub(b, a));
}

static inline Vec3f Vec3f_LineSegDir(Vec3f a, Vec3f b) {
    return Vec3f_Normalize(Vec3f_Sub(b, a));
}

static inline Vec3s Vec3s_LineSegDir(Vec3s a, Vec3s b) {
    return Vec3s_Normalize(Vec3s_Sub(b, a));
}

static inline Vec2f Vec2f_Project(Vec2f a, Vec2f b) {
    f32 ls = Vec2f_MagnitudeSQ(b);
    
    return Vec2f_MulVal(b, Vec2f_Dot(b, a) / ls);
}

static inline Vec3f Vec3f_Project(Vec3f a, Vec3f b) {
    f32 ls = Vec3f_MagnitudeSQ(b);
    
    return Vec3f_MulVal(b, Vec3f_Dot(b, a) / ls);
}

static inline Vec3s Vec3s_Project(Vec3s a, Vec3s b) {
    f32 ls = Vec3s_MagnitudeSQ(b);
    
    return Vec3s_MulVal(b, Vec3s_Dot(b, a) / ls);
}

static inline Vec2f Vec2f_Invert(Vec2f a) {
    return Vec2f_MulVal(a, -1);
}

static inline Vec3f Vec3f_Invert(Vec3f a) {
    return Vec3f_MulVal(a, -1);
}

static inline Vec3s Vec3s_Invert(Vec3s a) {
    return Vec3s_MulVal(a, -1);
}

static inline Vec2f Vec2f_InvMod(Vec2f a) {
    Vec2f r;
    
    r.x = 1.0f - fabsf(a.x);
    r.y = 1.0f - fabsf(a.y);
    
    return r;
}

static inline Vec3f Vec3f_InvMod(Vec3f a) {
    Vec3f r;
    
    r.x = 1.0f - fabsf(a.x);
    r.y = 1.0f - fabsf(a.y);
    r.z = 1.0f - fabsf(a.z);
    
    return r;
}

static inline f32 Vec2f_Cos(Vec2f a, Vec2f b) {
    f32 mp = Vec2f_Magnitude(a) * Vec2f_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Vec2f_Dot(a, b) / mp;
}

static inline f32 Vec3f_Cos(Vec3f a, Vec3f b) {
    f32 mp = Vec3f_Magnitude(a) * Vec3f_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Vec3f_Dot(a, b) / mp;
}

static inline f32 Vec3s_Cos(Vec3s a, Vec3s b) {
    f32 mp = Vec3s_Magnitude(a) * Vec3s_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Vec3s_Dot(a, b) / mp;
}

static inline Vec2f Vec2f_Reflect(Vec2f vec, Vec2f normal) {
    Vec2f negVec = Vec2f_Invert(vec);
    f32 vecDotNormal = Vec2f_Cos(negVec, normal);
    Vec2f normalScale = Vec2f_MulVal(normal, vecDotNormal);
    Vec2f nsVec = Vec2f_Add(normalScale, vec);
    
    return Vec2f_Add(negVec, Vec2f_MulVal(nsVec, 2.0f));
}

static inline Vec3f Vec3f_Reflect(Vec3f vec, Vec3f normal) {
    Vec3f negVec = Vec3f_Invert(vec);
    f32 vecDotNormal = Vec3f_Cos(negVec, normal);
    Vec3f normalScale = Vec3f_MulVal(normal, vecDotNormal);
    Vec3f nsVec = Vec3f_Add(normalScale, vec);
    
    return Vec3f_Add(negVec, Vec3f_MulVal(nsVec, 2.0f));
}

static inline Vec3s Vec3s_Reflect(Vec3s vec, Vec3s normal) {
    Vec3s negVec = Vec3s_Invert(vec);
    f32 vecDotNormal = Vec3s_Cos(negVec, normal);
    Vec3s normalScale = Vec3s_MulVal(normal, vecDotNormal);
    Vec3s nsVec = Vec3s_Add(normalScale, vec);
    
    return Vec3s_Add(negVec, Vec3s_MulVal(nsVec, 2.0f));
}

static inline Vec3f Vec3f_ClosestPointOnLine(Vec3f point, Vec3f lineA, Vec3f lineB) {
    Vec3f seg = Vec3f_LineSegDir(lineA, lineB);
    Vec3f proj = Vec3f_Project(point, seg);
    
    return Vec3f_Add(lineA, proj);
}

#pragma GCC diagnostic pop

#endif