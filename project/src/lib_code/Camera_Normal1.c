#include "Camera.h"

/*
   z64ram = 0x800473A0
   z64rom = 0xABE540
 */

s32 Camera_Normal1(Camera* camera) {
	Vec3f* eye = &camera->eye;
	Vec3f* at = &camera->at;
	Vec3f* eyeNext = &camera->eyeNext;
	f32 spA0;
	f32 sp9C;
	f32 sp98;
	f32 sp94;
	Vec3f sp88;
	s16 t;
	VecSph eyeAdjustment;
	VecSph atEyeGeo;
	VecSph atEyeNextGeo;
	Normal1* norm1 = (Normal1*)camera->paramData;
	Normal1Anim* anim = &norm1->anim;
	f32 playerHeight;
	f32 rate = 0.1f;
	
	playerHeight = Player_GetHeight(camera->player);
	if (RELOAD_PARAMS) {
		CameraModeValue* values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;
		f32 yNormal = (1.0f + PCT(R_CAM_YOFFSET_NORM) - PCT(R_CAM_YOFFSET_NORM) * (68.0f / playerHeight));
		sp94 = yNormal * PCT(playerHeight);
		
		norm1->yOffset = NEXTSETTING * sp94;
		norm1->distMin = NEXTSETTING * sp94;
		norm1->distMax = NEXTSETTING * sp94;
		norm1->pitchTarget = CAM_DEG_TO_BINANG(NEXTSETTING);
		norm1->unk_0C = NEXTSETTING;
		norm1->unk_10 = NEXTSETTING;
		norm1->unk_14 = NEXTPCT;
		norm1->fovTarget = NEXTSETTING;
		norm1->atLERPScaleMax = NEXTPCT;
		norm1->interfaceFlags = NEXTSETTING;
	}
	
	if (R_RELOAD_CAM_PARAMS) {
		Camera_CopyPREGToModeValues(camera);
	}
	
	sCameraInterfaceFlags = norm1->interfaceFlags;
	
	OLib_Vec3fDiffToVecSphGeo(&atEyeGeo, at, eye);
	OLib_Vec3fDiffToVecSphGeo(&atEyeNextGeo, at, eyeNext);
	
	switch (camera->animState) {
		case 0x14:
			camera->yawUpdateRateInv = OREG(27);
			camera->pitchUpdateRateInv = OREG(27);
		case 0:
		case 0xA:
		case 0x19:
			anim->swing.atEyePoly = NULL;
			anim->slopePitchAdj = 0;
			anim->unk_28 = 0xA;
			anim->swing.unk_16 = anim->swing.unk_14 = anim->swing.unk_18 = 0;
			anim->swing.swingUpdateRate = norm1->unk_0C;
			anim->yOffset = camera->playerPosRot.pos.y;
			anim->unk_20 = camera->xzSpeed;
			anim->swing.swingUpdateRateTimer = 0;
			anim->swingYawTarget = atEyeGeo.yaw;
			sUpdateCameraDirection = 0;
			anim->startSwingTimer = OREG(50) + OREG(51);
			break;
		default:
			break;
	}
	
	camera->animState = 1;
	sUpdateCameraDirection = 1;
	
	if (anim->unk_28 != 0) {
		anim->unk_28--;
	}
	
	if (camera->xzSpeed > 0.001f) {
		anim->startSwingTimer = OREG(50) + OREG(51);
	} else if (anim->startSwingTimer > 0) {
		if (anim->startSwingTimer > OREG(50)) {
			anim->swingYawTarget = atEyeGeo.yaw + (BINANG_SUB(BINANG_ROT180(camera->playerPosRot.rot.y), atEyeGeo.yaw) /
				anim->startSwingTimer);
		}
		// No Swing
		// anim->startSwingTimer--;
	}
	
	spA0 = camera->speedRatio * PCT(OREG(25));
	sp9C = camera->speedRatio * PCT(OREG(26));
	sp98 = anim->swing.unk_18 != 0 ? PCT(OREG(25)) : spA0;
	
	sp94 = (camera->xzSpeed - anim->unk_20) * (0.333333f);
	if (sp94 > 1.0f) {
		sp94 = 1.0f;
	}
	if (sp94 > -1.0f) {
		sp94 = -1.0f;
	}
	
	anim->unk_20 = camera->xzSpeed;
	
	if (anim->swing.swingUpdateRateTimer != 0) {
		camera->yawUpdateRateInv =
			Camera_LERPCeilF(
			anim->swing.swingUpdateRate + (f32)(anim->swing.swingUpdateRateTimer * 2),
			camera->yawUpdateRateInv,
			sp98,
			rate
			);
		camera->pitchUpdateRateInv =
			Camera_LERPCeilF(
			(f32)R_CAM_DEFA_PHI_UPDRATE + (f32)(anim->swing.swingUpdateRateTimer * 2),
			camera->pitchUpdateRateInv,
			sp9C,
			rate
			);
		anim->swing.swingUpdateRateTimer--;
	} else {
		camera->yawUpdateRateInv =
			Camera_LERPCeilF(
			anim->swing.swingUpdateRate - ((OREG(49) * 0.01f) * anim->swing.swingUpdateRate * sp94),
			camera->yawUpdateRateInv,
			sp98,
			rate
			);
		camera->pitchUpdateRateInv = Camera_LERPCeilF(R_CAM_DEFA_PHI_UPDRATE, camera->pitchUpdateRateInv, sp9C, rate);
	}
	
	camera->pitchUpdateRateInv = Camera_LERPCeilF(R_CAM_DEFA_PHI_UPDRATE, camera->pitchUpdateRateInv, sp9C, rate);
	camera->xzOffsetUpdateRate = Camera_LERPCeilF(PCT(OREG(2)), camera->xzOffsetUpdateRate, spA0, rate);
	camera->yOffsetUpdateRate = Camera_LERPCeilF(PCT(OREG(3)), camera->yOffsetUpdateRate, sp9C, rate);
	camera->fovUpdateRate = Camera_LERPCeilF(PCT(OREG(4)), camera->yOffsetUpdateRate, camera->speedRatio * 0.05f, rate);
	
	if (norm1->interfaceFlags & 1) {
		t = func_80044ADC(camera, BINANG_ROT180(atEyeGeo.yaw), 0);
		sp9C = ((1.0f / norm1->unk_10) * 0.5f) * (1.0f - camera->speedRatio);
		anim->slopePitchAdj = Camera_LERPCeilS(t, anim->slopePitchAdj, ((1.0f / norm1->unk_10) * 0.5f) + sp9C, 0xF);
	} else {
		anim->slopePitchAdj = 0;
		if (camera->playerGroundY == camera->playerPosRot.pos.y) {
			anim->yOffset = camera->playerPosRot.pos.y;
		}
	}
	
	spA0 = ((anim->swing.unk_18 != 0) && (norm1->yOffset > -40.0f))
	       ? (sp9C = Math_SinS(anim->swing.unk_14), ((-40.0f * sp9C) + (norm1->yOffset * (1.0f - sp9C))))
	       : norm1->yOffset;
	
	if (norm1->interfaceFlags & 0x80) {
		func_800458D4(camera, &atEyeNextGeo, spA0, &anim->yOffset, norm1->interfaceFlags & 1);
	} else if (norm1->interfaceFlags & 0x20) {
		func_80045B08(camera, &atEyeNextGeo, spA0, anim->slopePitchAdj);
	} else {
		Camera_CalcAtDefault(camera, &atEyeNextGeo, spA0, norm1->interfaceFlags & 1);
	}
	
	OLib_Vec3fDiffToVecSphGeo(&eyeAdjustment, at, eyeNext);
	
	camera->dist = eyeAdjustment.r =
		Camera_ClampDist(camera, eyeAdjustment.r, norm1->distMin, norm1->distMax, anim->unk_28);
	
	if (anim->startSwingTimer <= 0) {
		eyeAdjustment.pitch = atEyeNextGeo.pitch;
		eyeAdjustment.yaw =
			Camera_LERPCeilS(anim->swingYawTarget, atEyeNextGeo.yaw, 1.0f / camera->yawUpdateRateInv, 0xA);
	} else if (anim->swing.unk_18 != 0) {
		eyeAdjustment.yaw =
			Camera_LERPCeilS(anim->swing.unk_16, atEyeNextGeo.yaw, 1.0f / camera->yawUpdateRateInv, 0xA);
		eyeAdjustment.pitch =
			Camera_LERPCeilS(anim->swing.unk_14, atEyeNextGeo.pitch, 1.0f / camera->yawUpdateRateInv, 0xA);
	} else {
		// rotate yaw to follow player.
		eyeAdjustment.yaw =
			Camera_CalcDefaultYaw(camera, atEyeNextGeo.yaw, camera->playerPosRot.rot.y, norm1->unk_14, sp94);
		eyeAdjustment.pitch =
			Camera_CalcDefaultPitch(camera, atEyeNextGeo.pitch, norm1->pitchTarget, anim->slopePitchAdj);
	}
	
	// set eyeAdjustment pitch from 79.65 degrees to -85 degrees
	if (eyeAdjustment.pitch > 0x38A4) {
		eyeAdjustment.pitch = 0x38A4;
	}
	if (eyeAdjustment.pitch < -0x3C8C) {
		eyeAdjustment.pitch = -0x3C8C;
	}
	
	Camera_Vec3fVecSphGeoAdd(eyeNext, at, &eyeAdjustment);
	if ((camera->status == CAM_STAT_ACTIVE) && (!(norm1->interfaceFlags & 0x10))) {
		anim->swingYawTarget = BINANG_ROT180(camera->playerPosRot.rot.y);
		if (anim->startSwingTimer > 0) {
			func_80046E20(camera, &eyeAdjustment, norm1->distMin, norm1->unk_0C, &sp98, &anim->swing);
		} else {
			sp88 = *eyeNext;
			anim->swing.swingUpdateRate = camera->yawUpdateRateInv = norm1->unk_0C * 2.0f;
			if (Camera_BGCheck(camera, at, &sp88)) {
				anim->swingYawTarget = atEyeNextGeo.yaw;
				anim->startSwingTimer = -1;
			} else {
				*eye = *eyeNext;
			}
			anim->swing.unk_18 = 0;
		}
		
		if (anim->swing.unk_18 != 0) {
			camera->inputDir.y =
				Camera_LERPCeilS(
				camera->inputDir.y + BINANG_SUB(BINANG_ROT180(anim->swing.unk_16), camera->inputDir.y),
				camera->inputDir.y,
				1.0f - (0.99f * sp98),
				0xA
				);
		}
		
		if (norm1->interfaceFlags & 4) {
			camera->inputDir.x = -atEyeGeo.pitch;
			camera->inputDir.y = BINANG_ROT180(atEyeGeo.yaw);
			camera->inputDir.z = 0;
		} else {
			OLib_Vec3fDiffToVecSphGeo(&eyeAdjustment, eye, at);
			camera->inputDir.x = eyeAdjustment.pitch;
			camera->inputDir.y = eyeAdjustment.yaw;
			camera->inputDir.z = 0;
		}
	} else {
		anim->swing.swingUpdateRate = norm1->unk_0C;
		anim->swing.unk_18 = 0;
		sUpdateCameraDirection = 0;
		*eye = *eyeNext;
	}
	
	spA0 = (gSaveContext.health <= 16 ? 0.8f : 1.0f);
	camera->fov = Camera_LERPCeilF(norm1->fovTarget * spA0, camera->fov, camera->fovUpdateRate, 1.0f);
	camera->roll = Camera_LERPCeilS(0, camera->roll, 0.5f, 0xA);
	camera->atLERPStepScale = Camera_ClampLERPScale(camera, norm1->atLERPScaleMax);
	
	return 1;
}
