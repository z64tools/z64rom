#include <oot_mq_debug/z64hdr.h>

struct Time;

void uLib_DebugMessages(u32 msgID);

void uObject_LoadObjects(ObjectContext* objectCtx);
void* uObject_MagicFunc(ObjectContext* objectCtx, s32 bankIndex, s16 objectId);
s32 uObject_Spawn(ObjectContext* objectCtx, s16 objectId);
void uObject_UpdateBank(ObjectContext* objectCtx);

s32 Vanilla_HylianShieldCondition(Player* this);

u32 PlayerLib_InitSkelanime(PlayState* playState, u8* segment, SkelAnime* skelAnime);

s32 Actor_TalkCondition(Actor* actor, PlayState* playState, f32 arg2, f32 arg3, u32 exchangeItemId);

Actor* Overlay_ActorSpawn(ActorContext* actorCtx, PlayState* playState, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params);
void Overlay_EffectSpawn(PlayState* playState, s32 type, s32 priority, void* initParams);

void NewPlay_SpawnScene(PlayState* playState, s32 sceneNum, s32 spawn);
void NewPlay_Init(PlayState* playState);
void NewPlay_Draw(PlayState* playState);
void NewPlay_Update(PlayState* playState);
void NewPlay_Main(PlayState* playState);
void NewPlay_SetFadeOut(PlayState* play);
void NewPlay_SetupRespawn(PlayState* this, s32 respawnMode, s32 playerParams);
struct Time Play_GetTime(void);

void NewRoom_Draw(PlayState* globalCtx, Room* room, u32 flags);