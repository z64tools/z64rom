#include <uLib.h>
#include <code/sys_matrix.h>
#include <uLib_vector.h>

f32 MaxF(f32 a, f32 b) {
    return a >= b ? a : b;
}

f32 MinF(f32 a, f32 b) {
    return a < b ? a : b;
}

s32 MaxS(s32 a, s32 b) {
    return a >= b ? a : b;
}

s32 MinS(s32 a, s32 b) {
    return a < b ? a : b;
}

s32 WrapS(s32 x, s32 min, s32 max) {
    s32 range = max - min;
    
    if (x < min)
        x += range * ((min - x) / range + 1);
    
    return min + (x - min) % range;
}

f32 WrapF(f32 x, f32 min, f32 max) {
    f64 range = max - min;
    
    if (x < min)
        x += range * roundf((min - x) / range + 1);
    
    return min + fmodf((x - min), range);
}

f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
    f32 a = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
    f32 b = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
    f32 c = (x1 - xm1) * 0.5f;
    
    return (((((a * k) + b) * k) + c) * k) + x0;
}

f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
    f32 coeff[4];
    
    coeff[0] = (1.0f - k) * (1.0f - k) * (1.0f - k) / 6.0f;
    coeff[1] = k * k * k / 2.0f - k * k + 2.0f / 3.0f;
    coeff[2] = -k * k * k / 2.0f + k * k / 2.0f + k / 2.0f + 1.0f / 6.0f;
    coeff[3] = k * k * k / 6.0f;
    
    return (coeff[0] * xm1) + (coeff[1] * x0) + (coeff[2] * x1) + (coeff[3] * x2);
}

Vec3f Math_Vec3f_Spline1(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2) {
    //crustify
	return (Vec3f) {
       Math_Spline1(k, xm1.x, x0.x, x1.x, x2.x),
       Math_Spline1(k, xm1.y, x0.y, x1.y, x2.y),
       Math_Spline1(k, xm1.z, x0.z, x1.z, x2.z),
	};
    //uncrustify
}

Vec3f Math_Vec3f_Spline2(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2) {
    //crustify
	return (Vec3f) {
       Math_Spline2(k, xm1.x, x0.x, x1.x, x2.x),
       Math_Spline2(k, xm1.y, x0.y, x1.y, x2.y),
       Math_Spline2(k, xm1.z, x0.z, x1.z, x2.z),
	};
    //uncrustify
}

Vec3f Math_Vec3f_YawDist(f32 dist, s16 yaw) {
    Vec3f ret = {
        .x = Math_SinS(yaw) * dist,
        .z = Math_CosS(yaw) * dist,
    };
    
    return ret;
}

Vec3f Math_Vec3f_YawPitchDist(f32 dist, s16 yaw, s16 pitch) {
    Vec3f ret = {
        .x = Math_SinS(yaw) * dist,
        .y = Math_SinS(-pitch) * dist,
        .z = Math_CosS(yaw) * dist,
    };
    
    return ret;
}

Vec3f Math_Vec3f_PosRelativeTo(Vec3f* target, Vec3f* origin, s16 originYaw) {
    f32 cosRot2Y;
    f32 sinRot2Y;
    f32 deltaX;
    f32 deltaZ;
    
    cosRot2Y = Math_CosS(originYaw);
    sinRot2Y = Math_SinS(originYaw);
    deltaX = target->x - origin->x;
    deltaZ = target->z - origin->z;
    
    //crustify
	return (Vec3f) {
		.x = (deltaX * cosRot2Y) - (deltaZ * sinRot2Y),
		.z = (deltaX * sinRot2Y) + (deltaZ * cosRot2Y),
		.y = target->y - origin->y
	};
    //uncrustify
}

f32 Math_Vec3f_Length(Vec3f* a) {
    return sqrtf(SQ(a->x) + SQ(a->y) + SQ(a->z));
}

f32 PowF(f32 a, f32 b) {
    if (b == 1.0f) return a;
    if (b == 2.0f) return a * a;
    if (b == -1.0f) return 1.0f / a;
    if (b == 0.0f) return 1.0f;
    
    f32 low, high, sqr, acc, mid;
    
    if (b >= 1) {
        sqr = PowF(a, b / 2);
        
        return sqr * sqr;
    } else {
        low = 0.0f;
        high = 1.0f;
        sqr = sqrtf(a);
        acc = sqr;
        mid = high / 2;
        
        while (ABS(mid - b) > 0.000001f) {
            sqr = sqrtf(sqr);
            
            if (mid <= b) {
                low = mid;
                acc *= sqr;
            } else {
                high = mid;
                acc *= (1 / sqr);
            }
            mid = (low + high) / 2;
        }
        
        return acc;
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # Matrix                              #
// # # # # # # # # # # # # # # # # # # # #

void Matrix_RotateX_s(s16 binang, MatrixMode mtxMode) {
    Matrix_RotateX(BINANG_TO_RAD(binang), mtxMode);
}

void Matrix_RotateY_s(s16 binang, MatrixMode mtxMode) {
    Matrix_RotateY(BINANG_TO_RAD(binang), mtxMode);
}

void Matrix_RotateZ_s(s16 binang, MatrixMode mtxMode) {
    Matrix_RotateZ(BINANG_TO_RAD(binang), mtxMode);
}

void Matrix_RotateX_f(f32 deg, MatrixMode mtxMode) {
    Matrix_RotateX(DEG_TO_RAD(deg), mtxMode);
}

void Matrix_RotateY_f(f32 deg, MatrixMode mtxMode) {
    Matrix_RotateY(DEG_TO_RAD(deg), mtxMode);
}

void Matrix_RotateZ_f(f32 deg, MatrixMode mtxMode) {
    Matrix_RotateZ(DEG_TO_RAD(deg), mtxMode);
}

void Matrix_MultX(f32 x, Vec3f* dst) {
    MtxF* cmf = sCurrentMatrix;
    
    dst->x = cmf->wx + cmf->xx * x;
    dst->y = cmf->wy + cmf->xy * x;
    dst->z = cmf->wz + cmf->xz * x;
}

void Matrix_MultY(f32 y, Vec3f* dst) {
    MtxF* cmf = sCurrentMatrix;
    
    dst->x = cmf->wx + cmf->yx * y;
    dst->y = cmf->wy + cmf->yy * y;
    dst->z = cmf->wz + cmf->yz * y;
}

void Matrix_MultZ(f32 z, Vec3f* dst) {
    MtxF* cmf = sCurrentMatrix;
    
    dst->x = cmf->wx + cmf->zx * z;
    dst->y = cmf->wy + cmf->zy * z;
    dst->z = cmf->wz + cmf->zz * z;
}

s32 Actor_FocusPlayer(PlayState* play, Actor* this, Vec3s* headVector, f32 dist) {
    Player* p = GET_PLAYER(play);
    s16 y = Math_Vec3f_Yaw(&this->focus.pos, &p->actor.focus.pos);
    s16 x = Math_Vec3f_Pitch(&this->focus.pos, &p->actor.focus.pos);
    
    if (this->xzDistToPlayer < dist && ABS((s16)(this->shape.rot.y - this->yawTowardsPlayer)) < DEG_TO_BINANG(90)) {
        y = CLAMP((s16)(y - this->shape.rot.y), DEG_TO_BINANG(-60), DEG_TO_BINANG(60));
        x = CLAMP(x, DEG_TO_BINANG(-30), DEG_TO_BINANG(30));
        Math_ApproachS(&headVector->x, x, 7, 2000);
        Math_ApproachS(&headVector->y, y, 7, 2000);
        
        return 1;
    } else {
        Math_ApproachS(&headVector->x, 0, 7, 2000);
        Math_ApproachS(&headVector->y, 0, 7, 2000);
        
        return 0;
    }
    
    return 0;
}

// # # # # # # # # # # # # # # # # # # # #
// # Verlet                              #
// # # # # # # # # # # # # # # # # # # # #

Particle Particle_New(Vec3f pos, f32 mass) {
    return (Particle) {
               pos, pos, {}, mass
    };
}

void Particle_Update(Particle* particle, f32 gravity, Vec3f addForce, f32 c) {
    Vec3f prevPos;
    Vec3f force = {
        c * particle->vel.x + addForce.x + 0,
        c * particle->vel.y + addForce.y + gravity,
        c * particle->vel.z + addForce.z + 0
    };
    Vec3f accel = {
        force.x / particle->mass,
        force.y / particle->mass,
        force.z / particle->mass,
    };
    
    prevPos = particle->pos;
    
    particle->pos.x = particle->pos.x * 2 - particle->prevPos.x + accel.x;
    particle->pos.y = particle->pos.y * 2 - particle->prevPos.y + accel.y;
    particle->pos.z = particle->pos.z * 2 - particle->prevPos.z + accel.z;
    particle->prevPos = prevPos;
    
    Math_Vec3f_Diff(&particle->pos, &particle->prevPos, &particle->vel);
}

Chain Chain_New(Particle* p1, Particle* p2, f32 length) {
    return (Chain) {
               p1, p2, length
    };
}

/*
   Updates both particles (p1, p2) through stepping to the solution
 */
void Chain_UpdateStep(Chain* chain, f32 step, f32 max) {
    Vec3f diff;
    Vec3f target = {};
    f32 factor;
    f32 diffLength;
    
    diff = Vec3f_Sub(chain->p1->pos, chain->p2->pos);
    diffLength = Vec3f_DistXYZ(chain->p1->pos, chain->p2->pos);
    factor = (chain->length - diffLength) / diffLength;
    diff = Vec3f_MulVal(diff, factor * 0.5);
    
    Math_ApproachF(&target.x, diff.x, max, step);
    Math_ApproachF(&target.y, diff.y, max, step);
    Math_ApproachF(&target.z, diff.z, max, step);
    chain->p1->pos = Vec3f_Add(chain->p1->pos, target);
    chain->p2->pos = Vec3f_Sub(chain->p2->pos, target);
}

/*
   Updates both particles (p1, p2), gets the average best position for them
 */
void Chain_UpdateAverage(Chain* chain) {
    Vec3f diff;
    f32 factor;
    f32 diffLength;
    
    Math_Vec3f_Diff(&chain->p1->pos, &chain->p2->pos, &diff);
    diffLength = Math_Vec3f_DistXYZ(&chain->p1->pos, &chain->p2->pos);
    
    factor = (chain->length - diffLength) / diffLength * 0.5;
    
    chain->p1->pos.x += diff.x * factor;
    chain->p1->pos.y += diff.y * factor;
    chain->p1->pos.z += diff.z * factor;
    
    chain->p2->pos.x -= diff.x * factor;
    chain->p2->pos.y -= diff.y * factor;
    chain->p2->pos.z -= diff.z * factor;
}

/*
   Updates only particle2 (p2), keeping the shape relative to the p1 absolute
 */
void Chain_Update(Chain* chain) {
    Vec3f diff;
    f32 factor;
    f32 diffLength;
    
    Math_Vec3f_Diff(&chain->p1->pos, &chain->p2->pos, &diff);
    diffLength = Math_Vec3f_DistXYZ(&chain->p1->pos, &chain->p2->pos);
    
    factor = (chain->length - diffLength) / diffLength;
    
    chain->p2->pos.x -= diff.x * factor;
    chain->p2->pos.y -= diff.y * factor;
    chain->p2->pos.z -= diff.z * factor;
}
