#include <uLib.h>
#include <code/z_camera.h>
#include <code/z_camera_data.h>

Asm_VanillaHook(Camera_Update);
Vec3s Camera_Update(Camera* camera) {
    Vec3f viewAt;
    Vec3f viewEye;
    Vec3f viewUp;
    f32 viewFov;
    Vec3f pos;
    s32 bgId;
    f32 playerGroundY;
    f32 playerXZSpeed;
    VecSph eyeAtAngle;
    s16 bgCamIndex;
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
        pos = curPlayerPosRot.pos;
        pos.y += Player_GetHeight(camera->player);
        
        playerGroundY = BgCheck_EntityRaycastDown5(
            camera->play, &camera->play->colCtx, &playerFloorPoly, &bgId,
            &camera->player->actor, &pos
        );
        if (playerGroundY != BGCHECK_Y_MIN) {
            // player is above ground.
            camera->floorNorm.x = COLPOLY_GET_NORMAL(playerFloorPoly->normal.x);
            camera->floorNorm.y = COLPOLY_GET_NORMAL(playerFloorPoly->normal.y);
            camera->floorNorm.z = COLPOLY_GET_NORMAL(playerFloorPoly->normal.z);
            camera->bgId = bgId;
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
            camera->nextBgCamIndex = -1;
        }
        
        if (
            (camera->unk_14C & 1) && (camera->unk_14C & 4) && !(camera->unk_14C & 0x400) &&
            (!(camera->unk_14C & 0x200) || (player->currentBoots == PLAYER_BOOTS_IRON)) &&
            (!(camera->unk_14C & (s16)0x8000)) && (playerGroundY != BGCHECK_Y_MIN)
        ) {
            bgCamIndex = Camera_GetBgCamIndex(camera, &bgId, playerFloorPoly);
            if (bgCamIndex != -1) {
                camera->nextBgId = bgId;
                if (bgId == BGCHECK_SCENE) {
                    camera->nextBgCamIndex = bgCamIndex;
                }
            }
        }
        
        if (
            camera->nextBgCamIndex != -1 && (fabsf(curPlayerPosRot.pos.y - playerGroundY) < 2.0f) &&
            (!(camera->unk_14C & 0x200) || (player->currentBoots == PLAYER_BOOTS_IRON))
        ) {
            camera->bgId = camera->nextBgId;
            Camera_ChangeBgCamIndex(camera, camera->nextBgCamIndex);
            camera->nextBgCamIndex = -1;
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
        if ((gSaveContext.gameMode != GAMEMODE_NORMAL) && (gSaveContext.gameMode != GAMEMODE_END_CREDITS)) {
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
    
    // setting bgId to the ret of Quake_Calc, and checking that
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
            &viewUp, eyeAtAngle.pitch + quake.rotZ, eyeAtAngle.yaw + quake.unk_1A,
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
    
    if ((camera->play->sceneId == SCENE_SPOT00) && (camera->fov < 59.0f)) {
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

Asm_VanillaHook(Camera_Normal1);
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
    // PosRot* playerPosRot = &camera->playerPosRot;
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
        //     rwData->swingYawTarget = atEyeGeo.yaw + ((s16)((s16)(camera->playerPosRot.rot.y - 0x7FFF) - atEyeGeo.yaw) /
        //                                              rwData->startSwingTimer);
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
            camera->yawUpdateRateInv, sp98, rate
            );
        camera->pitchUpdateRateInv =
            Camera_LERPCeilF(
            (f32)R_CAM_DEFAULT_PITCH_UPDATE_RATE_INV + (f32)(rwData->swing.swingUpdateRateTimer * 2),
            camera->pitchUpdateRateInv, sp9C, rate
            );
        rwData->swing.swingUpdateRateTimer--;
    } else {
        camera->yawUpdateRateInv = Camera_LERPCeilF(
            rwData->swing.swingUpdateRate -
            ((OREG(49) * 0.01f) * rwData->swing.swingUpdateRate * sp94),
            camera->yawUpdateRateInv, sp98, rate
        );
        camera->pitchUpdateRateInv =
            Camera_LERPCeilF(R_CAM_DEFAULT_PITCH_UPDATE_RATE_INV, camera->pitchUpdateRateInv, sp9C, rate);
    }
    
    camera->pitchUpdateRateInv =
        Camera_LERPCeilF(R_CAM_DEFAULT_PITCH_UPDATE_RATE_INV, camera->pitchUpdateRateInv, sp9C, rate);
    camera->xzOffsetUpdateRate = Camera_LERPCeilF(CAM_DATA_SCALED(OREG(2)), camera->xzOffsetUpdateRate, spA0, rate);
    camera->yOffsetUpdateRate = Camera_LERPCeilF(CAM_DATA_SCALED(OREG(3)), camera->yOffsetUpdateRate, sp9C, rate);
    camera->fovUpdateRate =
        Camera_LERPCeilF(CAM_DATA_SCALED(OREG(4)), camera->yOffsetUpdateRate, camera->speedRatio * 0.05f, rate);
    
    if (roData->interfaceFlags & 1) {
        t = Camera_GetPitchAdjFromFloorHeightDiffs(camera, atEyeGeo.yaw - 0x7FFF, false);
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
        rwData->swingYawTarget = camera->playerPosRot.rot.y - 0x7FFF;
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
            camera->inputDir.y =
                Camera_LERPCeilS(
                camera->inputDir.y + (s16)((s16)(rwData->swing.unk_16 - 0x7FFF) - camera->inputDir.y),
                camera->inputDir.y, 1.0f - (0.99f * sp98), 0xA
                );
        }
        
        if (roData->interfaceFlags & 4) {
            camera->inputDir.x = -atEyeGeo.pitch;
            camera->inputDir.y = atEyeGeo.yaw - 0x7FFF;
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
