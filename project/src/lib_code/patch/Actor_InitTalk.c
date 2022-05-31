#include <uLib.h>

/*
   z64ram = 0x8002F1C4
   z64rom = 0xAA6364
   z64next = 0x8002F298
 */

s32 func_8002F1C4(Actor* actor, GlobalContext* globalCtx, f32 arg2, f32 arg3, u32 exchangeItemId) {
	return Actor_TalkCondition(actor, globalCtx, arg2, arg3, exchangeItemId);
}