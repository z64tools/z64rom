#include <uLib.h>

/*
   z64ram = 0x80097C00
   z64rom = 0xB0EDA0
 */

s32 Object_Spawn(ObjectContext* objectCtx, s16 objectId) {
	return uObject_Spawn(objectCtx, objectId);
}