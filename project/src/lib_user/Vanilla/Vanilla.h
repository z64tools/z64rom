#include <oot_mq_debug/z64hdr.h>

void uLib_DebugMessages(u32 msgID);

void uObject_LoadObjects(ObjectContext* objectCtx);
void* uObject_MagicFunc(ObjectContext* objectCtx, s32 bankIndex, s16 objectId);
s32 uObject_Spawn(ObjectContext* objectCtx, s16 objectId);
void uObject_UpdateBank(ObjectContext* objectCtx);

u32 PlayerLib_InitSkelanime(GlobalContext* globalCtx, u8* segment, SkelAnime* skelAnime);

Actor* Overlay_ActorSpawn(ActorContext* actorCtx, GlobalContext* globalCtx, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);
void Overlay_EffectSpawn(GlobalContext* globalCtx, s32 type, s32 priority, void* initParams);

void Play_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn);
void Play_Init(GlobalContext* globalCtx);
void Play_Draw(GlobalContext* globalCtx);
void Play_Update(GlobalContext* globalCtx);
void Play_Main(GlobalContext* globalCtx);