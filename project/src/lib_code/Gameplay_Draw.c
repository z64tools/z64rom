#include <uLib.h>

/*
   z64ram = 0x800BEDD8
   z64rom = 0xB35F78
 */

void Gameplay_Draw(GlobalContext* globalCtx) {
	GraphicsContext* gfxCtx = globalCtx->state.gfxCtx;
	
	gSegments[4] = VIRTUAL_TO_PHYSICAL(globalCtx->objectCtx.status[globalCtx->objectCtx.mainKeepIndex].segment);
	gSegments[5] = VIRTUAL_TO_PHYSICAL(globalCtx->objectCtx.status[globalCtx->objectCtx.subKeepIndex].segment);
	gSegments[2] = VIRTUAL_TO_PHYSICAL(globalCtx->sceneSegment);
	
	gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
	gSPSegment(POLY_XLU_DISP++, 0x00, NULL);
	gSPSegment(OVERLAY_DISP++, 0x00, NULL);
	
	gSPSegment(POLY_OPA_DISP++, 0x04, globalCtx->objectCtx.status[globalCtx->objectCtx.mainKeepIndex].segment);
	gSPSegment(POLY_XLU_DISP++, 0x04, globalCtx->objectCtx.status[globalCtx->objectCtx.mainKeepIndex].segment);
	gSPSegment(OVERLAY_DISP++, 0x04, globalCtx->objectCtx.status[globalCtx->objectCtx.mainKeepIndex].segment);
	
	gSPSegment(POLY_OPA_DISP++, 0x05, globalCtx->objectCtx.status[globalCtx->objectCtx.subKeepIndex].segment);
	gSPSegment(POLY_XLU_DISP++, 0x05, globalCtx->objectCtx.status[globalCtx->objectCtx.subKeepIndex].segment);
	gSPSegment(OVERLAY_DISP++, 0x05, globalCtx->objectCtx.status[globalCtx->objectCtx.subKeepIndex].segment);
	
	gSPSegment(POLY_OPA_DISP++, 0x02, globalCtx->sceneSegment);
	gSPSegment(POLY_XLU_DISP++, 0x02, globalCtx->sceneSegment);
	gSPSegment(OVERLAY_DISP++, 0x02, globalCtx->sceneSegment);
	
	func_80095248(gfxCtx, 0, 0, 0);
	
	if ((HREG(80) != 10) || (HREG(82) != 0))
		Play_Draw(globalCtx);
	
	if (globalCtx->view.unk_124 != 0) {
		Camera_Update(GET_ACTIVE_CAM(globalCtx));
		View_UpdateViewingMatrix(&globalCtx->view);
		globalCtx->view.unk_124 = 0;
		if (globalCtx->skyboxId && (globalCtx->skyboxId != SKYBOX_UNSET_1D) && !globalCtx->envCtx.skyboxDisabled) {
			SkyboxDraw_UpdateMatrix(
				&globalCtx->skyboxCtx,
				globalCtx->view.eye.x,
				globalCtx->view.eye.y,
				globalCtx->view.eye.z
			);
		}
	}
	
	Camera_Finish(GET_ACTIVE_CAM(globalCtx));
}
