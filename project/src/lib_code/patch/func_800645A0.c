#include <uLib.h>

/*
   z64ram = 0x800645A0
   z64rom = 0xADB740
 */

typedef void (* CutsceneStateHandler)(GlobalContext*, CutsceneContext*);
extern CutsceneStateHandler sCsStateHandlers2[];
asm ("sCsStateHandlers2 = 0x8011E1DC;");

void func_800645A0(GlobalContext* globalCtx, CutsceneContext* csCtx) {
#if 0
	Input* input = &globalCtx->state.input[0];
	if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT) && (csCtx->state == CS_STATE_IDLE) &&
		(gSaveContext.sceneSetupIndex >= 4)) {
		D_8015FCC8 = 0;
		gSaveContext.cutsceneIndex = 0xFFFD;
		gSaveContext.cutsceneTrigger = 1;
	}
	
	if (CHECK_BTN_ALL(input->press.button, BTN_DUP) && (csCtx->state == CS_STATE_IDLE) &&
		(gSaveContext.sceneSetupIndex >= 4) && !gDbgCamEnabled) {
		D_8015FCC8 = 1;
		gSaveContext.cutsceneIndex = 0xFFFD;
		gSaveContext.cutsceneTrigger = 1;
	}
#endif
	
	if ((gSaveContext.cutsceneTrigger != 0) && (globalCtx->sceneLoadFlag == TRANS_TRIGGER_START)) {
		gSaveContext.cutsceneTrigger = 0;
	}
	
	if ((gSaveContext.cutsceneTrigger != 0) && (csCtx->state == CS_STATE_IDLE)) {
		gSaveContext.cutsceneIndex = 0xFFFD;
		gSaveContext.cutsceneTrigger = 1;
	}
	
	if (gSaveContext.cutsceneIndex >= 0xFFF0) {
		func_80068ECC(globalCtx, csCtx);
		sCsStateHandlers2[csCtx->state](globalCtx, csCtx);
	}
}