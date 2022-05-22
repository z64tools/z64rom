#include <ULib.h>
#include "vt.h"

asm ("D_801333D4 = 0x801333D4");
asm ("D_801333E0 = 0x801333E0");
asm ("D_801333E8 = 0x801333E8");

void ULib_Update(GameState* gameState) {
#ifdef DEV_BUILD
	if (CHK_ALL(press, BTN_Z) && CHK_ALL(cur, BTN_L | BTN_R)) {
		gSaveContext.gameMode = 0;
		SET_NEXT_GAMESTATE(gameState, Select_Init, SelectContext);
		gameState->running = false;
	}
#endif
	
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

void ULib_DmaDebug(DmaRequest* req) {
#ifdef DEV_BUILD
	u32 id = 0;
	DmaEntry* iter = gDmaDataTable;
	
	while (iter->vromEnd) {
		if (req->vromAddr >= iter->vromStart && req->vromAddr < iter->vromEnd) {
			break;
		}
		
		id++;
		iter++;
	}
	
	if (id == 0x14)
		return;
	
	if (req->filename && strlen(req->filename) > 0)
		osLibPrintf("" PRNT_GRAY "[" PRNT_REDD "%s" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]", req->filename, req->line);
	
	else
		osLibPrintf("" PRNT_GRAY " [" PRNT_REDD "null" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]", req->line);
	osLibPrintf(
		"Dma Request: vrom " PRNT_BLUE "%08X - %08X" PRNT_RSET " size: " PRNT_BLUE "%08X " PRNT_RSET "[ " PRNT_REDD "ID 0x%04X" PRNT_RSET " ]",
		req->vromAddr,
		req->vromAddr + req->size,
		req->size,
		id
	);
#endif
}

static void __p_osLibPrintf(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	
	_Printf(is_proutSyncPrintf, NULL, fmt, args);
	
	va_end(args);
}

void osLibPrintf(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	
	__p_osLibPrintf("" PRNT_GRAY "[" PRNT_BLUE ">" PRNT_GRAY "]: " PRNT_RSET);
	_Printf(is_proutSyncPrintf, NULL, fmt, args);
	__p_osLibPrintf("" PRNT_RSET "\n");
	
	va_end(args);
}

void ULib_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn) {
	SceneTableEntry* scene = &gSceneTable[sceneNum];
	
	osSyncPrintf("\n");
	osLibPrintf("Get Scene num " PRNT_YELW "0x%02X" PRNT_RSET " %08X > %08X\n", sceneNum, scene, gSceneTable);
}