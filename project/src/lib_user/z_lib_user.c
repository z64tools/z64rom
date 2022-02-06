#include <z_lib_user.h>

LibContext gLibCtx = {
	.myMagicValue = 0xDEADBEEF,
};

asm ("D_801333D4 = 0x801333D4");
asm ("D_801333E0 = 0x801333E0");
asm ("D_801333E8 = 0x801333E8");

void ULib_Update(GameState* gameState) {
	if (CHK_ALL(press, BTN_Z) && CHK_ALL(cur, BTN_L | BTN_R)) {
		gSaveContext.gameMode = 0;
		SET_NEXT_GAMESTATE(gameState, Select_Init, SelectContext);
		gameState->running = false;
	}
	
	/* Skip current textbox when pressing B */
	if (gSaveContext.gameMode == 0) {
		static s32 soundFlag;
		GlobalContext* globalCtx = &gGlobalContext;
		MessageContext* msgCtx = &globalCtx->msgCtx;
		
		msgCtx->unk_E3D6 = 1;
		
		if (soundFlag == 1 && (msgCtx->msgMode == 52 || msgCtx->unk_E3E4 == 0x30)) {
			Audio_PlaySoundGeneral(NA_SE_SY_MESSAGE_END, &D_801333D4, 4, &D_801333E0, &D_801333E0, &D_801333E8);
			soundFlag = 0;
		}
		
		#define msgCtx_DecodeCur *AVAL(msgCtx, u16, 0xE3D2)
		#define msgCtx_DecodeEnd *AVAL(msgCtx, u16, 0xE3D4)
		
		if (CHK_ALL(press, BTN_B) && globalCtx->msgCtx.msgMode == 6 && msgCtx_DecodeCur >= 1 && msgCtx_DecodeCur < msgCtx_DecodeEnd) {
			msgCtx_DecodeCur = msgCtx_DecodeEnd;
			soundFlag = 1;
		}
	}
}