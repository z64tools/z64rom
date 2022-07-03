#include <uLib.h>
#include "code/z_actor.h"

/*
   z64ram = 0x8002FBAC
   z64rom = 0xAA6D4C
   z64next = 0x80030488
 */

void Actor_DrawFaroresWindPointer(PlayState* play) {
	s32 lightRadius = -1;
	s32 params;
	
	params = gSaveContext.respawn[RESPAWN_MODE_TOP].data;
	
	if (params) {
		f32 yOffset = LINK_IS_ADULT ? 80.0f : 60.0f;
		f32 ratio = 1.0f;
		s32 alpha = 255;
		s32 temp = params - 40;
		
		if (temp < 0) {
			gSaveContext.respawn[RESPAWN_MODE_TOP].data = ++params;
			ratio = ABS(params) * 0.025f;
			D_8015BC14 = 60;
			D_8015BC18 = 1.0f;
		} else if (D_8015BC14) {
			D_8015BC14--;
		} else if (D_8015BC18 > 0.0f) {
			static Vec3f effectVel = { 0.0f, -0.05f, 0.0f };
			static Vec3f effectAccel = { 0.0f, -0.025f, 0.0f };
			static Color_RGBA8 effectPrimCol = { 255, 255, 255, 0 };
			static Color_RGBA8 effectEnvCol = { 100, 200, 0, 0 };
			Vec3f* curPos = &gSaveContext.respawn[RESPAWN_MODE_TOP].pos;
			Vec3f* nextPos = &gSaveContext.respawn[RESPAWN_MODE_DOWN].pos;
			f32 prevNum = D_8015BC18;
			Vec3f dist;
			f32 diff = Math_Vec3f_DistXYZAndStoreDiff(nextPos, curPos, &dist);
			Vec3f effectPos;
			f32 factor;
			f32 length;
			f32 dx;
			f32 speed;
			
			if (diff < 20.0f) {
				D_8015BC18 = 0.0f;
				Math_Vec3f_Copy(curPos, nextPos);
			} else {
				length = diff * (1.0f / D_8015BC18);
				speed = 20.0f / length;
				speed = CLAMP_MIN(speed, 0.05f);
				Math_StepToF(&D_8015BC18, 0.0f, speed);
				factor = (diff * (D_8015BC18 / prevNum)) / diff;
				curPos->x = nextPos->x + (dist.x * factor);
				curPos->y = nextPos->y + (dist.y * factor);
				curPos->z = nextPos->z + (dist.z * factor);
				length *= 0.5f;
				dx = diff - length;
				yOffset += sqrtf(SQ(length) - SQ(dx)) * 0.2f;
			}
			
			effectPos.x = curPos->x + Rand_CenteredFloat(6.0f);
			effectPos.y = curPos->y + 80.0f + (6.0f * Rand_ZeroOne());
			effectPos.z = curPos->z + Rand_CenteredFloat(6.0f);
			
			EffectSsKiraKira_SpawnDispersed(
				play,
				&effectPos,
				&effectVel,
				&effectAccel,
				&effectPrimCol,
				&effectEnvCol,
				1000,
				16
			);
			
			if (D_8015BC18 == 0.0f) {
				gSaveContext.respawn[RESPAWN_MODE_TOP] = gSaveContext.respawn[RESPAWN_MODE_DOWN];
				gSaveContext.respawn[RESPAWN_MODE_TOP].playerParams = 0x06FF;
				gSaveContext.respawn[RESPAWN_MODE_TOP].data = 40;
			}
			
			gSaveContext.respawn[RESPAWN_MODE_TOP].pos = *curPos;
		} else if (temp > 0) {
			Vec3f* curPos = &gSaveContext.respawn[RESPAWN_MODE_TOP].pos;
			f32 nextRatio = 1.0f - temp * 0.1f;
			f32 curRatio = 1.0f - (f32)(temp - 1) * 0.1f;
			Vec3f eye;
			Vec3f dist;
			f32 diff;
			
			if (nextRatio > 0.0f) {
				eye.x = play->view.eye.x;
				eye.y = play->view.eye.y - yOffset;
				eye.z = play->view.eye.z;
				diff = Math_Vec3f_DistXYZAndStoreDiff(&eye, curPos, &dist);
				diff = (diff * (nextRatio / curRatio)) / diff;
				curPos->x = eye.x + (dist.x * diff);
				curPos->y = eye.y + (dist.y * diff);
				curPos->z = eye.z + (dist.z * diff);
				gSaveContext.respawn[RESPAWN_MODE_TOP].pos = *curPos;
			}
			
			alpha = 255 - (temp * 30);
			
			if (alpha < 0) {
				gSaveContext.fw.set = 0;
				gSaveContext.respawn[RESPAWN_MODE_TOP].data = 0;
				alpha = 0;
			} else {
				gSaveContext.respawn[RESPAWN_MODE_TOP].data = ++params;
			}
			
			ratio = 1.0f + ((f32)temp * 0.2); // required to match
		}
		
		lightRadius = 500.0f * ratio;
		
		if (play->csCtx.state == CS_STATE_IDLE &&
			gSaveContext.respawn[RESPAWN_MODE_TOP].roomIndex == play->roomCtx.curRoom.num &&
			(
				gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex == gSaveContext.entranceIndex
			||
				gExitParam.respawn[RESPAWN_MODE_TOP].sceneIndex == gExitParam.exit.sceneIndex
			)) {
			f32 scale = 0.025f * ratio;
			
			POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_25);
			
			Matrix_Translate(
				((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x),
				((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y) + yOffset,
				((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.z),
				MTXMODE_NEW
			);
			Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
			Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
			Matrix_Push();
			
			gDPPipeSync(POLY_XLU_DISP++);
			gDPSetPrimColor(POLY_XLU_DISP++, 128, 128, 255, 255, 200, alpha);
			gDPSetEnvColor(POLY_XLU_DISP++, 100, 200, 0, 255);
			
			Matrix_RotateZ(BINANG_TO_RAD_ALT2((play->gameplayFrames * 1500) & 0xFFFF), MTXMODE_APPLY);
			gSPMatrix(
				POLY_XLU_DISP++,
				Matrix_NewMtx(play->state.gfxCtx, "../z_actor.c", 5458),
				G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
			);
			gSPDisplayList(POLY_XLU_DISP++, gEffFlash1DL);
			
			Matrix_Pop();
			Matrix_RotateZ(BINANG_TO_RAD_ALT2(~((play->gameplayFrames * 1200) & 0xFFFF)), MTXMODE_APPLY);
			
			gSPMatrix(
				POLY_XLU_DISP++,
				Matrix_NewMtx(play->state.gfxCtx, "../z_actor.c", 5463),
				G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH
			);
			gSPDisplayList(POLY_XLU_DISP++, gEffFlash1DL);
		}
		
		Lights_PointNoGlowSetInfo(
			&D_8015BC00,
			((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.x),
			((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y) + yOffset,
			((void)0, gSaveContext.respawn[RESPAWN_MODE_TOP].pos.z),
			255,
			255,
			255,
			lightRadius
		);
	}
}