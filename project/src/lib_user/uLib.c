#define __ULIB_C__
#include <uLib.h>
#include "vt.h"

asm ("D_801333D4 = 0x801333D4");
asm ("D_801333E0 = 0x801333E0");
asm ("D_801333E8 = 0x801333E8");
// char wow[0x100000];

void uLib_Update(GameState* gameState) {
	gSaveContext.language = 0;
#ifdef DEV_BUILD
	static s32 firstMessage;
	const char* state[] = {
		"" PRNT_REDD "false",
		"" PRNT_BLUE "true",
	};
	
	if (firstMessage == 0) {
		osLibPrintf("" PRNT_BLUE "--- [z64rom] ---\n");
		osLibPrintf("Vanilla Printf [L + D-UP]");
		osLibPrintf("Dma Info       [L + D-DOWN]");
		firstMessage++;
	}
	
	if (CHK_ALL(press, BTN_L | BTN_DUP)) {
		gLibCtx.state.vanillaOsPrintf ^= 1;
		osLibPrintf("Vanilla Messages: [%s" PRNT_RSET "]", state[gLibCtx.state.vanillaOsPrintf]);
	}
	
	if (CHK_ALL(press, BTN_L | BTN_DDOWN)) {
		gLibCtx.state.dmaLog ^= 1;
		osLibPrintf("Dma Info: [%s" PRNT_RSET "]", state[gLibCtx.state.dmaLog]);
	}
	
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
			Audio_PlaySys(NA_SE_SY_MESSAGE_END);
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

void* memset(void* m, int v, unsigned int s) {
	for (s32 i = 0; i < s; i++)
		((u8*)m)[i] = v;
	
	return NULL;
}

#ifdef DEV_BUILD

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

#endif