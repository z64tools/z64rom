#include <oot_mq_debug/z64hdr.h>

void uLib_DebugMessages(u32 msgID);
Actor* uLib_Actor_Spawn(ActorContext* actorCtx, GlobalContext* globalCtx, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);
void uLib_Gameplay_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn);
void uLib_EffectSs_Spawn(GlobalContext* globalCtx, s32 type, s32 priority, void* initParams);
