#include <uLib.h>
#include <code/z_camera.h>
#include <code/z_camera_data.h>

/*
   z64ram = 0x800591EC
   z64rom = 0xAD038C
   z64next = 0x80059EC8
 */

Vec3s Camera_Update(Camera* camera) {
	Vec3f viewAt;
	Vec3f viewEye;
	Vec3f viewUp;
	f32 viewFov;
	Vec3f spAC;
	s32 bgId;
	f32 playerGroundY;
	f32 playerXZSpeed;
	VecSph eyeAtAngle;
	s16 camDataIdx;
	PosRot curPlayerPosRot;
	QuakeCamCalc quake;
	Player* player;
	
	player = camera->play->cameraPtrs[CAM_ID_MAIN]->player;
	
	if (camera->status == CAM_STAT_CUT)
		return camera->inputDir;
	
	sUpdateCameraDirection = false;
	
	if (camera->player != NULL) {
		Actor_GetWorldPosShapeRot(&curPlayerPosRot, &camera->player->actor);
		camera->xzSpeed = playerXZSpeed = OLib_Vec3fDistXZ(&curPlayerPosRot.pos, &camera->playerPosRot.pos);
		
		camera->speedRatio =
			OLib_ClampMaxDist(playerXZSpeed / (func_8002DCE4(camera->player) * CAM_DATA_SCALED(OREG(8))), 1.0f);
		camera->playerPosDelta.x = curPlayerPosRot.pos.x - camera->playerPosRot.pos.x;
		camera->playerPosDelta.y = curPlayerPosRot.pos.y - camera->playerPosRot.pos.y;
		camera->playerPosDelta.z = curPlayerPosRot.pos.z - camera->playerPosRot.pos.z;
		spAC = curPlayerPosRot.pos;
		spAC.y += Player_GetHeight(camera->player);
		
		playerGroundY = BgCheck_EntityRaycastFloor5(
			camera->play,
			&camera->play->colCtx,
			&playerFloorPoly,
			&bgId,
			&camera->player->actor,
			&spAC
		);
		if (playerGroundY != BGCHECK_Y_MIN) {
			// player is above ground.
			camera->floorNorm.x = COLPOLY_GET_NORMAL(playerFloorPoly->normal.x);
			camera->floorNorm.y = COLPOLY_GET_NORMAL(playerFloorPoly->normal.y);
			camera->floorNorm.z = COLPOLY_GET_NORMAL(playerFloorPoly->normal.z);
			camera->bgCheckId = bgId;
			camera->playerGroundY = playerGroundY;
		} else {
			// player is not above ground.
			camera->floorNorm.x = 0.0;
			camera->floorNorm.y = 1.0f;
			camera->floorNorm.z = 0.0;
		}
		
		camera->playerPosRot = curPlayerPosRot;
		
		if (camera->status == CAM_STAT_ACTIVE) {
			Camera_UpdateWater(camera);
			Camera_UpdateHotRoom(camera);
		}
		
		if (!(camera->unk_14C & 4)) {
			camera->nextCamDataIdx = -1;
		}
		
		if ((camera->unk_14C & 1) && (camera->unk_14C & 4) && !(camera->unk_14C & 0x400) &&
			(!(camera->unk_14C & 0x200) || (player->currentBoots == PLAYER_BOOTS_IRON)) &&
			(!(camera->unk_14C & (s16)0x8000)) && (playerGroundY != BGCHECK_Y_MIN)) {
			camDataIdx = Camera_GetDataIdxForPoly(camera, &bgId, playerFloorPoly);
			if (camDataIdx != -1) {
				camera->nextBGCheckId = bgId;
				if (bgId == BGCHECK_SCENE) {
					camera->nextCamDataIdx = camDataIdx;
				}
			}
		}
		
		if (camera->nextCamDataIdx != -1 && (fabsf(curPlayerPosRot.pos.y - playerGroundY) < 2.0f) &&
			(!(camera->unk_14C & 0x200) || (player->currentBoots == PLAYER_BOOTS_IRON))) {
			camera->bgCheckId = camera->nextBGCheckId;
			Camera_ChangeDataIdx(camera, camera->nextCamDataIdx);
			camera->nextCamDataIdx = -1;
		}
	}
	Camera_PrintSettings(camera);
	Camera_DbgChangeMode(camera);
	
	if (camera->status == CAM_STAT_WAIT)
		return camera->inputDir;
	
	camera->unk_14A = 0;
	camera->unk_14C &= ~(0x400 | 0x20);
	camera->unk_14C |= 0x10;
	
	sCameraFunctions[sCameraSettings[camera->setting].cameraModes[camera->mode].funcIdx](camera);
	
	if (camera->status == CAM_STAT_ACTIVE) {
		if ((gSaveContext.gameMode != 0) && (gSaveContext.gameMode != 3)) {
			sCameraInterfaceFlags = 0;
			Camera_UpdateInterface(sCameraInterfaceFlags);
		} else if ((D_8011D3F0 != 0) && (camera->camId == CAM_ID_MAIN)) {
			D_8011D3F0--;
			sCameraInterfaceFlags = 0x3200;
			Camera_UpdateInterface(sCameraInterfaceFlags);
		} else if (camera->play->transitionMode != TRANS_MODE_OFF) {
			sCameraInterfaceFlags = 0xF200;
			Camera_UpdateInterface(sCameraInterfaceFlags);
		} else if (camera->play->csCtx.state != CS_STATE_IDLE) {
			sCameraInterfaceFlags = 0x3200;
			Camera_UpdateInterface(sCameraInterfaceFlags);
		} else {
			Camera_UpdateInterface(sCameraInterfaceFlags);
		}
	}
	
	OREG(0) &= ~8;
	
	if (camera->status == CAM_STAT_UNK3) {
		return camera->inputDir;
	}
	
	// setting bgCheckId to the ret of Quake_Calc, and checking that
	// is required, it doesn't make too much sense though.
	if ((bgId = Quake_Calc(camera, &quake), bgId != 0) && (camera->setting != CAM_SET_TURN_AROUND)) {
		viewAt.x = camera->at.x + quake.atOffset.x;
		viewAt.y = camera->at.y + quake.atOffset.y;
		viewAt.z = camera->at.z + quake.atOffset.z;
		viewEye.x = camera->eye.x + quake.eyeOffset.x;
		viewEye.y = camera->eye.y + quake.eyeOffset.y;
		viewEye.z = camera->eye.z + quake.eyeOffset.z;
		OLib_Vec3fDiffToVecSphGeo(&eyeAtAngle, &viewEye, &viewAt);
		Camera_CalcUpFromPitchYawRoll(
			&viewUp,
			eyeAtAngle.pitch + quake.rotZ,
			eyeAtAngle.yaw + quake.unk_1A,
			camera->roll
		);
		viewFov = camera->fov + CAM_BINANG_TO_DEG(quake.zoom);
	} else {
		viewAt = camera->at;
		viewEye = camera->eye;
		OLib_Vec3fDiffToVecSphGeo(&eyeAtAngle, &viewEye, &viewAt);
		Camera_CalcUpFromPitchYawRoll(&viewUp, eyeAtAngle.pitch, eyeAtAngle.yaw, camera->roll);
		viewFov = camera->fov;
	}
	
	if (camera->paramFlags & 4) {
		camera->paramFlags &= ~4;
		viewUp = camera->up;
	} else {
		camera->up = viewUp;
	}
	
	camera->skyboxOffset = quake.eyeOffset;
	
	Camera_UpdateDistortion(camera);
	
	if ((camera->play->sceneNum == SCENE_SPOT00) && (camera->fov < 59.0f)) {
		View_SetScale(&camera->play->view, 0.79f);
	} else {
		View_SetScale(&camera->play->view, 1.0f);
	}
	camera->play->view.fovy = viewFov;
	View_LookAt(&camera->play->view, &viewEye, &viewAt, &viewUp);
	camera->camDir.x = eyeAtAngle.pitch;
	camera->camDir.y = eyeAtAngle.yaw;
	camera->camDir.z = 0;
	
	if (sUpdateCameraDirection == 0) {
		camera->inputDir.x = eyeAtAngle.pitch;
		camera->inputDir.y = eyeAtAngle.yaw;
		camera->inputDir.z = 0;
	}
	
#ifdef DEV_BUILD
	if (camera->timer != -1 && CHECK_BTN_ALL(D_8015BD7C->state.input[0].press.button, BTN_DRIGHT)) {
		camera->timer = 0;
	}
#endif
	
	return camera->inputDir;
}