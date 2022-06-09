#include <uLib.h>

/*
   z64ram = 0x80031F50
   z64rom = 0xAA90F0
 */

Actor* Actor_Spawn(ActorContext* actorCtx, GlobalContext* globalCtx, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params) {
	osLibPrintf("Spawn Actor: 0x%04X::%04X", (u16)actorId, (u16)params);
	
	return Overlay_ActorSpawn(actorCtx, globalCtx, actorId, posX, posY, posZ, rotX, rotY, rotZ, params);
}