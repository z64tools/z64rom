#include <uLib.h>

/*
   z64ram = 0x800982FC
   z64rom = 0xB0F49C
 */

void* func_800982FC(ObjectContext* objectCtx, s32 bankIndex, s16 objectId) {
	return uObject_MagicFunc(objectCtx, bankIndex, objectId);
}