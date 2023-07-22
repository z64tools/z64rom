#ifndef __ULIB_H__
#define __ULIB_H__

// # # # # # # # # # # # # # # # # # # # #
// # User Library Patches                #
// # # # # # # # # # # # # # # # # # # # #

// New save patches
#define Patch_SaveStartEntrance 0xBB
#define Patch_SaveStartAge      LINK_AGE_CHILD
#define Patch_SaveStartTime     0x6AAB
#define Patch_SaveStartCsIndex  0xFFF1

// Wield Hylian shield like Kokiri shield
#define Patch_WieldHylianShieldLikeKokiriShield true

// Play cutscene after obtaining Silver Gauntlets
#define Patch_SilverGauntletObtainCS false

// Flush current textbox by pressing B
#define Patch_QuickText true

// Space allocated for GI models, expand if needed
#define Pathch_GetItem_SegmentSize 0x3008

// Extension, these can be adjusted if necessary
#define EXT_DMA_MAX    3800
#define EXT_ACTOR_MAX  1000
#define EXT_OBJECT_MAX 1000
#define EXT_SCENE_MAX  256
#define EXT_EFFECT_MAX 64

// # # # # # # # # # # # # # # # # # # # #
// # UserLibrary                         #
// # # # # # # # # # # # # # # # # # # # #

#include <z64hdr.h>

#include "uLib_macros.h"
#include "uLib_types.h"
#include "sfx_enum.h"

extern DmaEntry __ext_gDmaDataTable[EXT_DMA_MAX];
extern ActorOverlay __ext_gActorOverlayTable[EXT_ACTOR_MAX];
extern RomFile __ext_gObjectTable[EXT_OBJECT_MAX];
extern SceneTableEntry __ext_gSceneTable[EXT_SCENE_MAX];
extern EffectSsOverlay __ext_gEffectSsOverlayTable[EXT_EFFECT_MAX];
extern u8 gFontOrdering[];
extern Vec3f gZeroVec;

#ifndef NO_Z64ROM_EXT_TABLES
#define gDmaDataTable         __ext_gDmaDataTable
#define gActorOverlayTable    __ext_gActorOverlayTable
#define gObjectTable          __ext_gObjectTable
#define gSceneTable           __ext_gSceneTable
#define gEffectSsOverlayTable __ext_gEffectSsOverlayTable
#endif

extern u32 osMemSize;
extern PlayState gPlayState;
extern LibContext gLibCtx;
extern GraphicsContext* __gfxCtx;
extern ExitParam gExitParam;
extern f32 EPSILON;
asm ("gPlayState = 0x80212020 - 0x38000;");
asm ("__gfxCtx = gPlayState;");
asm ("gExitParam = gPlayState + 0x11E18;"); // 801EBE38
asm ("osMemSize = 0x80000318");

void uLib_Update(GameState* gameState);
void* memset(void* m, int v, unsigned int s);
void* memmove(void* dest, const void* src, size_t len);
f32 fmodf(f32, f32);

#define DEFAULT_REVERB 0.35f
#define DEFAULT_RANGE  800.0f

void SoundEffect_PlayOneshot(SoundFile id, f32 vol, f32 pitch, Vec3f* pos, f32 reverb, f32 range, void* origin);
void SoundEffect_PlayHeld(SoundFile id, f32 vol, f32 pitch, f32 stepRelease, Vec3f* pos, f32 reverb, f32 range, void* origin);
void SoundEffect_PlayDefaultOneshot(SoundFile id, Vec3f* pos, void* origin);
void SoundEffect_PlayDefaultHeld(SoundFile id, f32 stepRelease, Vec3f* pos, void* origin);
void SoundEffect_StopOneshot(SoundFile id, Vec3f* pos, void* origin);
typedef struct SoundEntity SoundEntity;
SoundEntity* SoundEntity_Register(void* origin);
void SoundEntity_Unregister(SoundEntity* sen);
void SoundEntity_SetFilter(SoundEntity* sen, s8 filter);

#ifdef DEV_BUILD

#define Assert(cond)  if (!(cond)) { char buffer[82]; sprintf(buffer, "%s\nline: %d", __FILE__, __LINE__); Fault_AddHungupAndCrashImpl("Assert("#cond ");", buffer); }
#define osInfo(title) "" PRNT_GRAY "[" PRNT_REDD "%s" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]" PRNT_RSET ": " PRNT_REDD title, __FUNCTION__, __LINE__

void Debug_Text(u8 r, u8 g, u8 b, s32 x, s32 y, char* fmt, ...);
void Debug_DmaLog(DmaRequest* req);
void DebugMenu_Update(PlayState* playState);
s32 DebugMenu_CineCamera(Camera* camera, Normal1* norm1, Player* player);
void Profiler_Start(DebugProfiler* profiler);
void Profiler_End(DebugProfiler* profiler);
void osLibPrintf(const char* fmt, ...);
void osLibHex(const char* txt, const void* data, u32 size, u32 dispOffset);

#else /* RELEASE_BUILD */

#define Debug_Text(...)           do {} while (0)
#define Debug_DmaLog(...)         do {} while (0)
#define DebugMenu_Update(...)     do {} while (0)
#define DebugMenu_CineCamera(...) do {} while (0)
#define Profiler_Start(...)       do {} while (0)
#define Profiler_End(...)         do {} while (0)
#define Assert(cond)              if (cond) (void)0;
#define osInfo(title)             do {} while (0)
#define osLibPrintf(...)          do {} while (0)
#define osLibHex(...)             do {} while (0)
#endif

void Audio_PlaySys(u16 flag);
Color_HSL Color_RgbToHsl(f32 r, f32 g, f32 b);
Color_RGB8 Color_HslToRgb(f32 h, f32 s, f32 l);
void Cutscene_PlaySegment(PlayState* play, void* segment);
Vtx* Gfx_AllocQuad(PlayState* playState, s16 x, s16 y, s16 width, s16 height, s16 u, s16 v);
s32 Lights_SortLightList(PlayState* play, LightInfo* sortedLightList[7]);
void Lights_SetPointlight(PlayState* play, Lights* lights, LightParams* params, bool isWiiVC);
void Lights_RebindActor(PlayState* play, Actor* actor, Vec3f* bindPos);
void Lights_RebindPointlightsActor(PlayState* play, Actor* actor, bool isWiiVC);

f32 MaxF(f32 a, f32 b);
f32 MinF(f32 a, f32 b);
s32 MaxS(s32 a, s32 b);
s32 MinS(s32 a, s32 b);
s32 WrapS(s32 x, s32 min, s32 max);
f32 WrapF(f32 x, f32 min, f32 max);
f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
Vec3f Math_Vec3f_Spline1(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2);
Vec3f Math_Vec3f_Spline2(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2);
Vec3f Math_Vec3f_YawDist(f32 dist, s16 yaw);
Vec3f Math_Vec3f_YawPitchDist(f32 dist, s16 yaw, s16 pitch);
Vec3f Math_Vec3f_PosRelativeTo(Vec3f* target, Vec3f* origin, s16 originYaw);
f32 Math_Vec3f_Length(Vec3f* a);
f32 PowF(f32 a, f32 b);

void Matrix_RotateX_s(s16 binang, MatrixMode mtxMode);
void Matrix_RotateY_s(s16 binang, MatrixMode mtxMode);
void Matrix_RotateZ_s(s16 binang, MatrixMode mtxMode);
void Matrix_RotateX_f(f32 deg, MatrixMode mtxMode);
void Matrix_RotateY_f(f32 deg, MatrixMode mtxMode);
void Matrix_RotateZ_f(f32 deg, MatrixMode mtxMode);
void Matrix_MultX(f32 x, Vec3f* dst);
void Matrix_MultY(f32 y, Vec3f* dst);
void Matrix_MultZ(f32 z, Vec3f* dst);

s32 Actor_FocusPlayer(PlayState* play, Actor* this, Vec3s* headVector, f32 dist);

Particle Particle_New(Vec3f pos, f32 mass);
void Particle_Update(Particle* particle, f32 gravity, Vec3f addForce, f32 c);
Chain Chain_New(Particle* p1, Particle* p2, f32 length);
void Chain_UpdateStep(Chain* chain, f32 step, f32 max);
void Chain_UpdateAverage(Chain* chain);
void Chain_Update(Chain* chain);

void* Segment_Scene_GetHeader(void* segment, s32 setupIndex);
void* Segment_Scene_GetCutscene(void* segment, s32 setupIndex);
CollisionHeader* Segment_Scene_GetCollision(void* segment, s32 setupIndex);
void Segment_Scene_PlayCutscene(void* segment, s32 setupIndex);
u8 PathList_GetNum(u8 index, s8 header);
void* PathList_GetList(u8 index, s8 header);

void SceneAnim_Update(PlayState* playState);

void SkelAnime_InterpFrameTable_Quat(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight);

void Play_SetFadeOut(PlayState* play);
struct Time Play_GetTime(void);
void NewRoom_Draw(PlayState* play, Room* room, u32 flags);

typedef enum {
    OVL_MSG_TALK,
    OVL_MSG_CHECK,
} OvlMsgType;

typedef enum {
    MSGBOX_TYPE_BLACK,
    MSGBOX_TYPE_WOODEN,
    MSGBOX_TYPE_BLUE,
    MSGBOX_TYPE_OCARINA,
    MSGBOX_TYPE_NONE_BOTTOM,
    MSGBOX_TYPE_NONE_NO_SHADOW,
    MSGBOX_TYPE_CREDITS = 11
} MsgBoxType;

typedef enum {
    MSGBOX_POS_VARIABLE,
    MSGBOX_POS_TOP,
    MSGBOX_POS_MIDDLE,
    MSGBOX_POS_BOTTOM
} MsgBoxPos;

#define MSG_COLOR_DEFAULT   "\x05\x00"
#define MSG_COLOR_RED       "\x05\x01"
#define MSG_COLOR_GREEN     "\x05\x02"
#define MSG_COLOR_BLUE      "\x05\x03"
#define MSG_COLOR_LIGHTBLUE "\x05\x04"
#define MSG_COLOR_PURPLE    "\x05\x05"
#define MSG_COLOR_YELLOW    "\x05\x06"
#define MSG_COLOR_BLACK     "\x05\x07"
#define MSG_COLOR_REG_0     "\x05\x08"
#define MSG_COLOR_REG_1     "\x05\x09"
#define MSG_COLOR_REG_2     "\x05\x0A"
#define MSG_COLOR_REG_3     "\x05\x0B"
#define MSG_COLOR_REG_4     "\x05\x0C"
#define MSG_COLOR_REG_5     "\x05\x0D"
#define MSG_COLOR_REG_6     "\x05\x0E"
#define MSG_COLOR_REG_7     "\x05\x0F"

#define MSG_SHIFT(x)             "\x06" x // 1
#define MSG_BOX_BREAK_DELAYED(x) "\x0C" x // 1
#define MSG_FADE(x)              "\x0E" x // 1
#define MSG_FADE2(x, y)          "\x11" x y // 2
#define MSG_SFX(x, y)            "\x12" x y // 2
#define MSG_ITEM_ICON(x)         "\x13" x // 1
#define MSG_TEXT_SPEED(x)        "\x14" x // 1
#define MSG_HIGHSCORE(x)         "\x1E" x // 1

#define MSG_INSTANT_ON    "\x08"
#define MSG_INSTANT_OFF   "\x09"
#define MSG_PERSISTENT    "\xFA"
#define MSG_EVENT         "\x0B"
#define MSG_NAME          "\x0F"
#define MSG_OCARINA       "\x10"
#define MSG_MARATHON_TIME "\x16"
#define MSG_RACE_TIME     "\x17"
#define MSG_POINTS        "\x18"
#define MSG_TOKENS        "\x19"
#define MSG_TWO_CHOICE    "\x02"
#define MSG_THREE_CHOICE  "\x03"
#define MSG_FISH_INFO     "\x1D"
#define MSG_TIME          "\x1F"

typedef struct OvlMessage {
    MsgBoxType type;
    MsgBoxPos  pos;
    char*      txt;
} OvlMessage;

void OvlMessage_Init(Actor* actor, OvlMsgType type);
void OvlMessage_RegisterColor(u8 id, u8 r, u8 g, u8 b);
s8 OvlMessage_Prompt(Actor* actor, OvlMessage* msg, f32 radius, u32 exchangeItemId);
void OvlMessage_Start(Actor* actor, OvlMessage* msg);
void OvlMessage_Continue(Actor* actor, OvlMessage* msg);
s8 OvlMessage_SwapFocus(Actor* actor);
s8 OvlMessage_IsClosed(Actor* actor);
s8 OvlMessage_IsCurrentMessage(OvlMessage* msg);
s8 OvlMessage_SetBranch(OvlMessage* branch);
s8 OvlMessage_GetChoice(Actor* actor);
s8 OvlMessage_GetBoxNum(void);

#endif