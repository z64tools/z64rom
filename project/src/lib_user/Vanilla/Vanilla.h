#include <oot_mq_debug/z64hdr.h>

void uLib_DebugMessages(u32 msgID);
Actor* Overlay_ActorSpawn(ActorContext* actorCtx, GlobalContext* globalCtx, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);
void Overlay_EffectSpawn(GlobalContext* globalCtx, s32 type, s32 priority, void* initParams);

void Play_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn);
void Play_Init(GlobalContext* globalCtx);
void Play_Draw(GlobalContext* globalCtx);
void Play_Update(GlobalContext* globalCtx);
void Play_Main(GlobalContext* globalCtx);