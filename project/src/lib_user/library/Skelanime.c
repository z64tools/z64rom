#include <uLib.h>

// Copyright (C) 2021 Sauraen

typedef struct {
    f32 w;
    f32 x;
    f32 y;
    f32 z;
} Quat;

#define Math_Atan2S_Swapped(y, x) Math_Atan2S(x, y)

static void Euler2Quat(Vec3s* src, Quat* dest) {
    float cx = Math_CosS(src->x / 2);
    float sx = Math_SinS(src->x / 2);
    float cy = Math_CosS(src->y / 2);
    float sy = Math_SinS(src->y / 2);
    float cz = Math_CosS(src->z / 2);
    float sz = Math_SinS(src->z / 2);
    
    dest->w = cx * cy * cz + sx * sy * sz;
    dest->x = sx * cy * cz - cx * sy * sz;
    dest->y = cx * sy * cz + sx * cy * sz;
    dest->z = cx * cy * sz - sx * sy * cz;
}

static void Quat2Euler(Quat* src, Vec3s* dest) {
    //Normalize the quaternion
    float mult = src->w * src->w + src->x * src->x + src->y * src->y + src->z * src->z;
    
    if (mult < 0.001f) {
        //This can occur when a new morph is started while another morph is
        //ongoing, corrupting the morph table. This check avoids a crash due to
        //divide-by-zero.
        //printf("output quaternion is 0");
        mult = 0.001f;
    }
    //Normally we would divide each component by 1 / sqrt(mult), but the
    //components are only ever used multiplied in pairs, so it becomes 1 / mult
    //and we factor that out. There's also a 2 wherever this ends up being used
    //in the equations below, so that's also factored out here.
    mult = 2.0f / mult;
    float temp = mult * (src->w * src->y - src->x * src->z);
    
    if (temp >= 1.0f) {
        dest->y = 0x4000;
    } else if (temp <= -1.0f) {
        dest->y = 0xC000;
    } else {
        dest->x = Math_Atan2S_Swapped(mult * (src->w * src->x + src->y * src->z), 1.0f - mult * (src->x * src->x + src->y * src->y));
        dest->y = Math_Atan2S_Swapped(temp, sqrtf(1.0f - temp * temp));
        dest->z = Math_Atan2S_Swapped(mult * (src->w * src->z + src->x * src->y), 1.0f - mult * (src->y * src->y + src->z * src->z));
        
        return;
    }
    //for both of the singularity cases above:
    dest->x = Math_Atan2S(src->x, src->w);
    dest->z = 0;
}

void SkelAnime_InterpFrameTable_Quat(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight) {
    if (weight >= 1.0f) {
        bcopy(target, dst, limbCount * sizeof(Vec3s));
        
        return;
    }
    if (weight <= 0.0f) {
        bcopy(start, dst, limbCount * sizeof(Vec3s));
        
        return;
    }
    for (s32 i = 0; i<limbCount; i++, dst++, start++, target++) {
        s32 numLargeRot = 0;
        s16 dx = target->x - start->x;
        s16 dy = target->y - start->y;
        s16 dz = target->z - start->z;
        if ((u16)dx <= 0xC000 || (u16)dx >= 0x4000) ++numLargeRot;
        if ((u16)dy <= 0xC000 || (u16)dy >= 0x4000) ++numLargeRot;
        if ((u16)dz <= 0xC000 || (u16)dz >= 0x4000) ++numLargeRot;
        if (numLargeRot >= 2 &&  i >= 1) {
            Quat qs, qt, qo;
            Euler2Quat(start, &qs);
            Euler2Quat(target, &qt);
            f32 cosHalfTheta = qs.w * qt.w + qs.x * qt.x + qs.y * qt.y + qs.z * qt.z;
            f32 wtmult = 1.0f;
            if (cosHalfTheta < 0.0f) {
                wtmult = -1.0f;
                cosHalfTheta = -cosHalfTheta;
            }
            f32 ws, wt;
            if (cosHalfTheta > 0.97f) {
                ws = 1.0f - weight;
                wt = weight;
            } else {
                f32 sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);
                s16 halfTheta = Math_Atan2S_Swapped(sinHalfTheta, cosHalfTheta);
                f32 rcpSinHalfTheta = 1.0f / sinHalfTheta;
                ws = Math_SinS((1.0f - weight) * halfTheta) * rcpSinHalfTheta;
                wt = Math_SinS(weight * halfTheta) * rcpSinHalfTheta;
            }
            wt *= wtmult;
            qo.w = ws * qs.w + wt * qt.w;
            qo.x = ws * qs.x + wt * qt.x;
            qo.y = ws * qs.y + wt * qt.y;
            qo.z = ws * qs.z + wt * qt.z;
            Quat2Euler(&qo, dst);
        } else {
            dst->x = (s16)(dx * weight) + start->x;
            dst->y = (s16)(dy * weight) + start->y;
            dst->z = (s16)(dz * weight) + start->z;
        }
    }
}
