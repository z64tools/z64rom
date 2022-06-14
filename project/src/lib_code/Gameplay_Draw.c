#include <uLib.h>

/*
   z64ram = 0x800BEDD8
   z64rom = 0xB35F78
 */

void Gameplay_Draw(PlayState* playState) {
	GraphicsContext* gfxCtx = playState->state.gfxCtx;
	
	gSegments[4] = VIRTUAL_TO_PHYSICAL(playState->objectCtx.status[playState->objectCtx.mainKeepIndex].segment);
	gSegments[5] = VIRTUAL_TO_PHYSICAL(playState->objectCtx.status[playState->objectCtx.subKeepIndex].segment);
	gSegments[2] = VIRTUAL_TO_PHYSICAL(playState->sceneSegment);
	
	gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
	gSPSegment(POLY_XLU_DISP++, 0x00, NULL);
	gSPSegment(OVERLAY_DISP++, 0x00, NULL);
	
	gSPSegment(POLY_OPA_DISP++, 0x04, playState->objectCtx.status[playState->objectCtx.mainKeepIndex].segment);
	gSPSegment(POLY_XLU_DISP++, 0x04, playState->objectCtx.status[playState->objectCtx.mainKeepIndex].segment);
	gSPSegment(OVERLAY_DISP++, 0x04, playState->objectCtx.status[playState->objectCtx.mainKeepIndex].segment);
	
	gSPSegment(POLY_OPA_DISP++, 0x05, playState->objectCtx.status[playState->objectCtx.subKeepIndex].segment);
	gSPSegment(POLY_XLU_DISP++, 0x05, playState->objectCtx.status[playState->objectCtx.subKeepIndex].segment);
	gSPSegment(OVERLAY_DISP++, 0x05, playState->objectCtx.status[playState->objectCtx.subKeepIndex].segment);
	
	gSPSegment(POLY_OPA_DISP++, 0x02, playState->sceneSegment);
	gSPSegment(POLY_XLU_DISP++, 0x02, playState->sceneSegment);
	gSPSegment(OVERLAY_DISP++, 0x02, playState->sceneSegment);
	
	func_80095248(gfxCtx, 0, 0, 0);
	
	if ((HREG(80) != 10) || (HREG(82) != 0))
		NewPlay_Draw(playState);
	
	if (playState->view.unk_124 != 0) {
		Camera_Update(GET_ACTIVE_CAM(playState));
		View_UpdateViewingMatrix(&playState->view);
		playState->view.unk_124 = 0;
		if (playState->skyboxId && (playState->skyboxId != SKYBOX_UNSET_1D) && !playState->envCtx.skyboxDisabled) {
			SkyboxDraw_UpdateMatrix(
				&playState->skyboxCtx,
				playState->view.eye.x,
				playState->view.eye.y,
				playState->view.eye.z
			);
		}
	}
	
	Camera_Finish(GET_ACTIVE_CAM(playState));
}
