#include <uLib.h>
#include <code/sys_matrix.h>

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
	Vec3f ret = { 0 };
	
	ret.x += Math_SinS(yaw) * dist;
	ret.z += Math_CosS(yaw) * dist;
	
	return ret;
}

Vec3f Math_Vec3f_YawPitchDist(f32 dist, s16 yaw, s16 pitch) {
	Vec3f ret = { 0 };
	
	ret.x += Math_SinS(yaw) * dist;
	ret.z += Math_CosS(yaw) * dist;
	ret.y += Math_SinS(-pitch) * dist;
	
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

void Matrix_RotateX_f(f32 binang, MatrixMode mtxMode) {
	Matrix_RotateX(DEG_TO_RAD(binang), mtxMode);
}

void Matrix_RotateY_f(f32 binang, MatrixMode mtxMode) {
	Matrix_RotateY(DEG_TO_RAD(binang), mtxMode);
}

void Matrix_RotateZ_f(f32 binang, MatrixMode mtxMode) {
	Matrix_RotateZ(DEG_TO_RAD(binang), mtxMode);
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

// # # # # # # # # # # # # # # # # # # # #
// # Physics                             #
// # # # # # # # # # # # # # # # # # # # #

void Physics_GetHeadProperties(PhysicsStrand* strand, Vec3f* headPosModel) {
	Vec3f zero = { 0 };
	
	Matrix_Get(&strand->head.mtxF);
	Matrix_MtxFToYXZRotS(&strand->head.mtxF, &strand->head.rot, 0);
	if (headPosModel == NULL) {
		Matrix_MultVec3f(&zero, &strand->head.pos);
		
		return;
	}
	Matrix_MultVec3f(headPosModel, &strand->head.pos);
}

void Physics_SetPhysicsStrand(PhysicsStrandInit* init, PhysicsStrand* dest, f32* limbLengthDest, Vec3f* spheresCenters, s32 spheresArrayCount, f32 spheresRadius) {
	dest->info = init->info;
	dest->gfx = init->gfx;
	dest->constraint = init->constraint;
	dest->rigidity = init->rigidity;
	dest->head = (PhysicsHead) { 0 };
	
	for (s32 i = 0; i < init->info.numLimbs + 1; i++) {
		limbLengthDest[i] = init->limbsLength[i];
	}
	dest->limbsLength = limbLengthDest;
	dest->spheres.centers = spheresCenters;
	dest->spheres.num = spheresArrayCount;
	dest->spheres.radius = spheresRadius;
}

void Physics_DrawDynamicStrand(GraphicsContext* gfxCtx, TwoHeadGfxArena* disp, PhysicsLimb* jointTable, PhysicsStrand* strand, void* callback, void* callbackArg1, void* callbackArg2) {
	s32 i;
	f32 tempY;
	f32 angX;
	f32 angY;
	Vec3f workVec;
	Vec3f posAdd;
	Vec3f rigidity, smoothedRigid;
	Vec3f velAdj = { 0 };
	Vec3f* pPos;
	Vec3f* pRot;
	Vec3f* pVel;
	Vec3f* pPrevPos;
	Vec3f* pPrevRot;
	
	Matrix_Push();
	
	jointTable[0].pos.x = strand->head.pos.x;
	jointTable[0].pos.y = strand->head.pos.y;
	jointTable[0].pos.z = strand->head.pos.z;
	
	for (i = 1; i < strand->info.numLimbs + 1; i++) {
		Math_ApproachF(&jointTable[i].vel.x, 0.0f, 1.0f, strand->info.velStep);
		Math_ApproachF(&jointTable[i].vel.y, 0.0f, 1.0f, strand->info.velStep);
		Math_ApproachF(&jointTable[i].vel.z, 0.0f, 1.0f, strand->info.velStep);
	}
	
	Matrix_RotateX_s(strand->head.rot.x, MTXMODE_NEW);
	Matrix_RotateY_s(strand->head.rot.y, MTXMODE_APPLY);
	Matrix_RotateZ_s(strand->head.rot.z, MTXMODE_APPLY);
	if (strand->rigidity.rot.x != 0) Matrix_RotateX_f(strand->rigidity.rot.x, MTXMODE_APPLY);
	if (strand->rigidity.rot.y != 0) Matrix_RotateY_f(strand->rigidity.rot.y, MTXMODE_APPLY);
	if (strand->rigidity.rot.z != 0) Matrix_RotateZ_f(strand->rigidity.rot.z, MTXMODE_APPLY);
	Matrix_MultVec3f(&strand->rigidity.push, &rigidity);
	
	// Main calculation loop
	for (i = 1; i < strand->info.numLimbs + 1; i++) {
		pPos = &jointTable[i].pos;
		pVel = &jointTable[i].vel;
		pPrevPos = &jointTable[i - 1].pos;
		pPrevRot = &jointTable[i - 1].rot;
		
		// Smoothens curve at the start of the limb array
		smoothedRigid = (Vec3f) { 0 };
		if (i < strand->rigidity.num) {
			smoothedRigid.x = ((strand->rigidity.num - i) * rigidity.x) * strand->rigidity.mult;
			smoothedRigid.y = ((strand->rigidity.num - i) * rigidity.y) * strand->rigidity.mult;
			smoothedRigid.z = ((strand->rigidity.num - i) * rigidity.z) * strand->rigidity.mult;
		}
		
		workVec.x = pPos->x + pVel->x - pPrevPos->x + smoothedRigid.x;
		tempY = pPos->y + pVel->y + strand->info.gravity + smoothedRigid.y;
		workVec.z = pPos->z + pVel->z - pPrevPos->z + smoothedRigid.z;
		
		// FLOOR, also gets rid of the smoothedRigid
		if (tempY < strand->info.floorY + 10.0f) {
			workVec.x -= smoothedRigid.x;
			workVec.z -= smoothedRigid.z;
			if (i != strand->info.numLimbs + 1 && tempY < strand->info.floorY)
				tempY = CLAMP_MIN(tempY, strand->info.floorY);
		}
		
		workVec.y = tempY - pPrevPos->y;
		
		angY = Math_Atan2F(workVec.z, workVec.x);
		angX = -Math_Atan2F(sqrtf(SQ(workVec.x) + SQ(workVec.z)), workVec.y);
		pPrevRot->y = angY;
		pPrevRot->x = angX;
		
		// Handle constraints if they are set
		if (strand->constraint.rotStepCalc.x != 0 || strand->constraint.rotStepCalc.y != 0 || strand->constraint.lockRoot) {
			s16 rootAngleX, rootAngleY;
			s16 workAngleX, workAngleY;
			s16 tempAngleX, tempAngleY;
			
			// Handle root rotations differently
			if (i == 1) {
				if (strand->constraint.lockRoot) {
					pPrevRot->x = angX = -Math_Atan2F(sqrtf(SQ(rigidity.x) + SQ(rigidity.z)), rigidity.y);
					pPrevRot->y = angY = Math_Atan2F(rigidity.z, rigidity.x);
				} else {
					if (strand->constraint.rotStepCalc.x != 0) {
						rootAngleX = RAD_TO_BINANG(-Math_Atan2F(sqrtf(SQ(rigidity.x) + SQ(rigidity.z)), rigidity.y));
						workAngleX = RAD_TO_BINANG(-Math_Atan2F(sqrtf(SQ(workVec.x) + SQ(workVec.z)), workVec.y));
						Math_SmoothStepToS(&rootAngleX, workAngleX, 1, DEG_TO_BINANG(strand->constraint.rotStepCalc.x), 1);
						angX = BINANG_TO_RAD(rootAngleX);
						pPrevRot->x = angX;
					}
					if (strand->constraint.rotStepCalc.y != 0) {
						rootAngleY = RAD_TO_BINANG(Math_Atan2F(rigidity.z, rigidity.x));
						workAngleY = RAD_TO_BINANG(Math_Atan2F(workVec.z, workVec.x));
						Math_SmoothStepToS(&rootAngleY, workAngleY, 1, DEG_TO_BINANG(strand->constraint.rotStepCalc.y), 1);
						angY = BINANG_TO_RAD(rootAngleY);
						pPrevRot->y = angY;
					}
				}
			} else {
				if (strand->constraint.rotStepCalc.x != 0) {
					tempAngleX = RAD_TO_BINANG(jointTable[i - 2].rot.x);
					workAngleX = RAD_TO_BINANG(-Math_Atan2F(sqrtf(SQ(workVec.x) + SQ(workVec.z)), workVec.y));
					Math_SmoothStepToS(&tempAngleX, workAngleX, 1, DEG_TO_BINANG(strand->constraint.rotStepCalc.x), 1);
					angX = BINANG_TO_RAD(tempAngleX);
					pPrevRot->x = angX;
				}
				if (strand->constraint.rotStepCalc.y != 0) {
					tempAngleY = RAD_TO_BINANG(jointTable[i - 2].rot.y);
					workAngleY = RAD_TO_BINANG(Math_Atan2F(workVec.z, workVec.x));
					Math_SmoothStepToS(&tempAngleY, workAngleY, 1, DEG_TO_BINANG(strand->constraint.rotStepCalc.y), 1);
					angY = BINANG_TO_RAD(tempAngleY);
					pPrevRot->y = angY;
				}
			}
		}
		
		Matrix_RotateY(angY, MTXMODE_NEW);
		Matrix_RotateX(angX, MTXMODE_APPLY);
		Matrix_MultZ(ABS(strand->limbsLength[i]) * strand->gfx.scale.z, &posAdd);
		
		// Pushes limbs away from selected Vec3f points
		for (s32 i = 0; i < strand->spheres.num; i++) {
			Vec3f tempPosAdd = {
				pPrevPos->x + posAdd.x,
				pPrevPos->y + posAdd.y,
				pPrevPos->z + posAdd.z
			};
			f32 radiusTo = Math_Vec3f_DistXYZ(&tempPosAdd, &strand->spheres.centers[i]);
			
			if (radiusTo < strand->spheres.radius) {
				s16 yaw = Math_Vec3f_Yaw(&strand->spheres.centers[i], &tempPosAdd);
				posAdd.x += Math_SinS(yaw) * (strand->spheres.radius - radiusTo) * 0.5f;
				posAdd.z += Math_CosS(yaw) * (strand->spheres.radius - radiusTo) * 0.5f;
				velAdj.x += Math_SinS(yaw) * (strand->spheres.radius - radiusTo) * 0.5f;
				velAdj.z += Math_CosS(yaw) * (strand->spheres.radius - radiusTo) * 0.5f;
				
				pPrevRot->y = angY =  Math_Atan2F(posAdd.z, posAdd.x);
				pPrevRot->x = angX = -Math_Atan2F(sqrtf(SQ(posAdd.x) + SQ(posAdd.z)), posAdd.y);
			}
		}
		
		workVec.x = pPos->x;
		workVec.y = pPos->y;
		workVec.z = pPos->z;
		
		pPos->x = pPrevPos->x + posAdd.x;
		pPos->y = pPrevPos->y + posAdd.y;
		pPos->z = pPrevPos->z + posAdd.z;
		
		pVel->x = (pPos->x - workVec.x + velAdj.x) * strand->info.velMult;
		pVel->y = (pPos->y - workVec.y + velAdj.y) * strand->info.velMult;
		pVel->z = (pPos->z - workVec.z + velAdj.z) * strand->info.velMult;
		
		jointTable[i].vel = (Vec3f) {
			CLAMP(pVel->x, -strand->info.maxVel, strand->info.maxVel),
			CLAMP(pVel->y, -strand->info.maxVel, strand->info.maxVel),
			CLAMP(pVel->z, -strand->info.maxVel, strand->info.maxVel),
		};
	}
	
	if (strand->gfx.noDraw) {
		Matrix_Pop();
		
		return;
	}
	
	Mtx* matrix = Graph_Alloc(gfxCtx, strand->info.numLimbs * sizeof(Mtx));
	s16 y = RAD_TO_BINANG(jointTable[0].rot.y);
	s16 x = RAD_TO_BINANG(jointTable[0].rot.x);
	
	for (i = 0; i < strand->info.numLimbs; i++) {
		pPos = &jointTable[i].pos;
		pRot = &jointTable[i].rot;
		
		Matrix_Translate(pPos->x, pPos->y, pPos->z, MTXMODE_NEW);
		
		if (strand->constraint.rotStepDraw.y != 0) {
			Math_SmoothStepToS(&y, RAD_TO_BINANG(pRot->y), 3, DEG_TO_BINANG(strand->constraint.rotStepDraw.y), 1);
			Matrix_RotateY_s(y, MTXMODE_APPLY);
		} else {
			Matrix_RotateY(pRot->y, MTXMODE_APPLY);
		}
		
		if (strand->constraint.rotStepDraw.x != 0) {
			Math_SmoothStepToS(&x, RAD_TO_BINANG(pRot->x), 3, DEG_TO_BINANG(strand->constraint.rotStepDraw.x), 1);
			Matrix_RotateX_s(x, MTXMODE_APPLY);
		} else {
			Matrix_RotateX(pRot->x, MTXMODE_APPLY);
		}
		
		if (strand->limbsLength[i] < 0)
			Matrix_Scale(-strand->gfx.scale.x, strand->gfx.scale.y, -strand->gfx.scale.z, MTXMODE_APPLY);
		else
			Matrix_Scale(strand->gfx.scale.x, strand->gfx.scale.y, strand->gfx.scale.z, MTXMODE_APPLY);
		
		if (callback != NULL) {
			((PhysicCallback)callback)(i, callbackArg1, callbackArg2);
		}
		Matrix_ToMtx(&matrix[i], __FILE__, __LINE__);
	}
	
	gDPPipeSync(disp->p++);
	gSPSegment(disp->p++, strand->gfx.segID, matrix);
	gSPDisplayList(disp->p++, strand->gfx.dlist);
	Matrix_Pop();
}