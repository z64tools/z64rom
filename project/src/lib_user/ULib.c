#include <ULib.h>
#include "vt.h"

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
		
		msgCtx->textUnskippable = 1;
		
		if (soundFlag == 1 && (msgCtx->msgMode == 52 || msgCtx->textboxEndType == 0x30)) {
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

void ULib_DmaDebug(DmaRequest* req, DmaEntry* dma) {
	const char** name = sDmaMgrFileNames;
	u32 id = 0;
	DmaEntry* iter = gDmaDataTable;
	u32* wow;
	
	while (iter->vromEnd) {
		if (req->vromAddr >= iter->vromStart && req->vromAddr < iter->vromEnd) {
			break;
		}
		
		iter++;
		name++;
	}
	
	wow = (u32*)*name;
	
	if (wow && wow[0] == 0x6C696E6B && wow[1] == 0x5F616E69)
		return;
	
	osSyncPrintf(VT_FGCOL(YELLOW));
	osSyncPrintf(">" VT_RST);
	osSyncPrintf(VT_FGCOL(CYAN));
	osSyncPrintf("Dma Request: " VT_RST);
	osSyncPrintf("vrom " VT_FGCOL(BLUE) "%08X - %08X" VT_RST " size: " VT_FGCOL(BLUE) "%08X " VT_RST, req->vromAddr, req->vromAddr + req->size, req->size);
	osSyncPrintf("[ " VT_FGCOL(RED) "%s" VT_RST " ]\n", *name);
	osSyncPrintf(VT_FGCOL(YELLOW));
	osSyncPrintf(" " VT_RST);
	osSyncPrintf(VT_FGCOL(CYAN));
	osSyncPrintf("Dma Entry:   " VT_RST);
	osSyncPrintf("vrom " VT_FGCOL(BLUE) "%08X - %08X" VT_RST " size: " VT_FGCOL(BLUE) "%08X " VT_RST, dma->vromStart, dma->vromEnd, dma->vromEnd - dma->romStart);
	if (dma->romEnd)
		osSyncPrintf("[ " VT_FGCOL(YELLOW) "Yaz" VT_RST " ]", *name);
	osSyncPrintf("\n");
}