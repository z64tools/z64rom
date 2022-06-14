#include <uLib.h>
#include "code/z_camera.h"
#include "code/z_camera_data.h"

/*
   z64ram = 0x800473A0
   z64rom = 0xABE540
   z64next = 0x80047F64
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
	s16 wiggleAdj;
	s16 t;
	VecSph eyeAdjustment;
	VecSph atEyeGeo;
	VecSph atEyeNextGeo;
	Normal1ReadOnlyData* roData = &camera->paramData.norm1.roData;
	Normal1ReadWriteData* rwData = &camera->paramData.norm1.rwData;
	f32 playerHeight;
	f32 rate = 0.1f;
	
	playerHeight = Player_GetHeight(camera->player);
	if (RELOAD_PARAMS(camera) || R_RELOAD_CAM_PARAMS) {
		CameraModeValue* values = sCameraSettings[camera->setting].cameraModes[camera->mode].values;
		f32 yNormal =
			(1.0f + CAM_DATA_SCALED(R_CAM_YOFFSET_NORM) - CAM_DATA_SCALED(R_CAM_YOFFSET_NORM) * (68.0f / playerHeight));
		
		sp94 = yNormal * CAM_DATA_SCALED(playerHeight);
		
		roData->yOffset = GET_NEXT_RO_DATA(values) * sp94;
		roData->distMin = GET_NEXT_RO_DATA(values) * sp94;
		roData->distMax = GET_NEXT_RO_DATA(values) * sp94;
		roData->pitchTarget = CAM_DEG_TO_BINANG(GET_NEXT_RO_DATA(values));
		roData->unk_0C = GET_NEXT_RO_DATA(values);
		roData->unk_10 = GET_NEXT_RO_DATA(values);
		roData->unk_14 = GET_NEXT_SCALED_RO_DATA(values);
		roData->fovTarget = GET_NEXT_RO_DATA(values);
		roData->atLERPScaleMax = GET_NEXT_SCALED_RO_DATA(values);
		roData->interfaceFlags = GET_NEXT_RO_DATA(values);
	}
	
	if (R_RELOAD_CAM_PARAMS) {
		Camera_CopyPREGToModeValues(camera);
	}
	
	sCameraInterfaceFlags = roData->interfaceFlags;
	
	OLib_Vec3fDiffToVecSphGeo(&atEyeGeo, at, eye);
	OLib_Vec3fDiffToVecSphGeo(&atEyeNextGeo, at, eyeNext);
	
	switch (camera->animState) {
		case 20:
			camera->yawUpdateRateInv = OREG(27);
			camera->pitchUpdateRateInv = OREG(27);
			FALLTHROUGH;
		case 0:
		case 10:
		case 25:
			rwData->swing.atEyePoly = NULL;
			rwData->slopePitchAdj = 0;
			rwData->unk_28 = 0xA;
			rwData->swing.unk_16 = rwData->swing.unk_14 = rwData->swing.unk_18 = 0;
			rwData->swing.swingUpdateRate = roData->unk_0C;
			rwData->yOffset = camera->playerPosRot.pos.y;
			rwData->unk_20 = camera->xzSpeed;
			rwData->swing.swingUpdateRateTimer = 0;
			rwData->swingYawTarget = atEyeGeo.yaw;
			sUpdateCameraDirection = 0;
			rwData->startSwingTimer = OREG(50) + OREG(51);
			break;
		default:
			break;
	}
	
	camera->animState = 1;
	sUpdateCameraDirection = 1;
	
	if (rwData->unk_28 != 0) {
		rwData->unk_28--;
	}
	
	if (camera->xzSpeed > 0.001f) {
		rwData->startSwingTimer = OREG(50) + OREG(51);
	} else if (rwData->startSwingTimer > 0) {
		// if (rwData->startSwingTimer > OREG(50)) {
		// 	rwData->swingYawTarget =
		// 		atEyeGeo.yaw +
		// 		(BINANG_SUB(BINANG_ROT180(camera->playerPosRot.rot.y), atEyeGeo.yaw) / rwData->startSwingTimer);
		// }
		// rwData->startSwingTimer--;
	}
	
	spA0 = camera->speedRatio * CAM_DATA_SCALED(OREG(25));
	sp9C = camera->speedRatio * CAM_DATA_SCALED(OREG(26));
	sp98 = rwData->swing.unk_18 != 0 ? CAM_DATA_SCALED(OREG(25)) : spA0;
	
	sp94 = (camera->xzSpeed - rwData->unk_20) * (0.333333f);
	if (sp94 > 1.0f) {
		sp94 = 1.0f;
	}
	if (sp94 > -1.0f) {
		sp94 = -1.0f;
	}
	
	rwData->unk_20 = camera->xzSpeed;
	
	if (rwData->swing.swingUpdateRateTimer != 0) {
		camera->yawUpdateRateInv =
			Camera_LERPCeilF(
			rwData->swing.swingUpdateRate + (f32)(rwData->swing.swingUpdateRateTimer * 2),
			camera->yawUpdateRateInv,
			sp98,
			rate
			);
		camera->pitchUpdateRateInv =
			Camera_LERPCeilF(
			(f32)R_CAM_DEFA_PHI_UPDRATE + (f32)(rwData->swing.swingUpdateRateTimer * 2),
			camera->pitchUpdateRateInv,
			sp9C,
			rate
			);
		rwData->swing.swingUpdateRateTimer--;
	} else {
		camera->yawUpdateRateInv = Camera_LERPCeilF(
			rwData->swing.swingUpdateRate -
			((OREG(49) * 0.01f) * rwData->swing.swingUpdateRate * sp94),
			camera->yawUpdateRateInv,
			sp98,
			rate
		);
		camera->pitchUpdateRateInv = Camera_LERPCeilF(R_CAM_DEFA_PHI_UPDRATE, camera->pitchUpdateRateInv, sp9C, rate);
	}
	
	camera->pitchUpdateRateInv = Camera_LERPCeilF(R_CAM_DEFA_PHI_UPDRATE, camera->pitchUpdateRateInv, sp9C, rate);
	camera->xzOffsetUpdateRate = Camera_LERPCeilF(CAM_DATA_SCALED(OREG(2)), camera->xzOffsetUpdateRate, spA0, rate);
	camera->yOffsetUpdateRate = Camera_LERPCeilF(CAM_DATA_SCALED(OREG(3)), camera->yOffsetUpdateRate, sp9C, rate);
	camera->fovUpdateRate =
		Camera_LERPCeilF(CAM_DATA_SCALED(OREG(4)), camera->yOffsetUpdateRate, camera->speedRatio * 0.05f, rate);
	
	if (roData->interfaceFlags & 1) {
		t = func_80044ADC(camera, BINANG_ROT180(atEyeGeo.yaw), 0);
		sp9C = ((1.0f / roData->unk_10) * 0.5f) * (1.0f - camera->speedRatio);
		rwData->slopePitchAdj =
			Camera_LERPCeilS(t, rwData->slopePitchAdj, ((1.0f / roData->unk_10) * 0.5f) + sp9C, 0xF);
	} else {
		rwData->slopePitchAdj = 0;
		if (camera->playerGroundY == camera->playerPosRot.pos.y) {
			rwData->yOffset = camera->playerPosRot.pos.y;
		}
	}
	
	spA0 = ((rwData->swing.unk_18 != 0) && (roData->yOffset > -40.0f))
		? (sp9C = Math_SinS(rwData->swing.unk_14), ((-40.0f * sp9C) + (roData->yOffset * (1.0f - sp9C))))
		: roData->yOffset;
	
	if (roData->interfaceFlags & 0x80) {
		func_800458D4(camera, &atEyeNextGeo, spA0, &rwData->yOffset, roData->interfaceFlags & 1);
	} else if (roData->interfaceFlags & 0x20) {
		func_80045B08(camera, &atEyeNextGeo, spA0, rwData->slopePitchAdj);
	} else {
		Camera_CalcAtDefault(camera, &atEyeNextGeo, spA0, roData->interfaceFlags & 1);
	}
	
	OLib_Vec3fDiffToVecSphGeo(&eyeAdjustment, at, eyeNext);
	
	camera->dist = eyeAdjustment.r =
		Camera_ClampDist(camera, eyeAdjustment.r, roData->distMin, roData->distMax, rwData->unk_28);
	
	if (rwData->startSwingTimer <= 0) {
		eyeAdjustment.pitch = atEyeNextGeo.pitch;
		eyeAdjustment.yaw =
			Camera_LERPCeilS(rwData->swingYawTarget, atEyeNextGeo.yaw, 1.0f / camera->yawUpdateRateInv, 0xA);
	} else if (rwData->swing.unk_18 != 0) {
		eyeAdjustment.yaw =
			Camera_LERPCeilS(rwData->swing.unk_16, atEyeNextGeo.yaw, 1.0f / camera->yawUpdateRateInv, 0xA);
		eyeAdjustment.pitch =
			Camera_LERPCeilS(rwData->swing.unk_14, atEyeNextGeo.pitch, 1.0f / camera->yawUpdateRateInv, 0xA);
	} else {
		// rotate yaw to follow player.
		eyeAdjustment.yaw =
			Camera_CalcDefaultYaw(camera, atEyeNextGeo.yaw, camera->playerPosRot.rot.y, roData->unk_14, sp94);
		eyeAdjustment.pitch =
			Camera_CalcDefaultPitch(camera, atEyeNextGeo.pitch, roData->pitchTarget, rwData->slopePitchAdj);
	}
	
	// set eyeAdjustment pitch from 79.65 degrees to -85 degrees
	if (eyeAdjustment.pitch > 0x38A4) {
		eyeAdjustment.pitch = 0x38A4;
	}
	if (eyeAdjustment.pitch < -0x3C8C) {
		eyeAdjustment.pitch = -0x3C8C;
	}
	
	Camera_Vec3fVecSphGeoAdd(eyeNext, at, &eyeAdjustment);
	if ((camera->status == CAM_STAT_ACTIVE) && !(roData->interfaceFlags & 0x10)) {
		rwData->swingYawTarget = BINANG_ROT180(camera->playerPosRot.rot.y);
		if (rwData->startSwingTimer > 0) {
			func_80046E20(camera, &eyeAdjustment, roData->distMin, roData->unk_0C, &sp98, &rwData->swing);
		} else {
			sp88 = *eyeNext;
			rwData->swing.swingUpdateRate = camera->yawUpdateRateInv = roData->unk_0C * 2.0f;
			if (Camera_BGCheck(camera, at, &sp88)) {
				rwData->swingYawTarget = atEyeNextGeo.yaw;
				rwData->startSwingTimer = -1;
			} else {
				*eye = *eyeNext;
			}
			rwData->swing.unk_18 = 0;
		}
		
		if (rwData->swing.unk_18 != 0) {
			camera->inputDir.y = Camera_LERPCeilS(
				camera->inputDir.y + BINANG_SUB(BINANG_ROT180(rwData->swing.unk_16), camera->inputDir.y),
				camera->inputDir.y,
				1.0f - (0.99f * sp98),
				0xA
			);
		}
		
		if (roData->interfaceFlags & 4) {
			camera->inputDir.x = -atEyeGeo.pitch;
			camera->inputDir.y = BINANG_ROT180(atEyeGeo.yaw);
			camera->inputDir.z = 0;
		} else {
			OLib_Vec3fDiffToVecSphGeo(&eyeAdjustment, eye, at);
			camera->inputDir.x = eyeAdjustment.pitch;
			camera->inputDir.y = eyeAdjustment.yaw;
			camera->inputDir.z = 0;
		}
		
		// crit wiggle
		if (gSaveContext.health <= 16 && ((camera->play->state.frames % 256) == 0)) {
			wiggleAdj = Rand_ZeroOne() * 10000.0f;
			camera->inputDir.y = wiggleAdj + camera->inputDir.y;
		}
	} else {
		rwData->swing.swingUpdateRate = roData->unk_0C;
		rwData->swing.unk_18 = 0;
		sUpdateCameraDirection = 0;
		*eye = *eyeNext;
	}
	
	spA0 = (gSaveContext.health <= 16 ? 0.8f : 1.0f);
	camera->fov = Camera_LERPCeilF(roData->fovTarget * spA0, camera->fov, camera->fovUpdateRate, 1.0f);
	camera->roll = Camera_LERPCeilS(0, camera->roll, 0.5f, 0xA);
	camera->atLERPStepScale = Camera_ClampLERPScale(camera, roData->atLERPScaleMax);
	
	return 1;
}
