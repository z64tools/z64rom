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

// Extension, these can be adjusted if necessary
#define EXT_DMA_MAX    3800
#define EXT_ACTOR_MAX  1000
#define EXT_OBJECT_MAX 1000
#define EXT_SCENE_MAX  256
#define EXT_EFFECT_MAX 64

// # # # # # # # # # # # # # # # # # # # #
// # UserLibrary                         #
// # # # # # # # # # # # # # # # # # # # #

#include <oot_mq_debug/z64hdr.h>

#include <library/Library.h>
#include <vanilla/Vanilla.h>

#include "uLib_macros.h"
#include "uLib_types.h"

extern DmaEntry __ext_gDmaDataTable[EXT_DMA_MAX];
extern ActorOverlay __ext_gActorOverlayTable[EXT_ACTOR_MAX];
extern RomFile __ext_gObjectTable[EXT_OBJECT_MAX];
extern SceneTableEntry __ext_gSceneTable[EXT_SCENE_MAX];
extern EffectSsOverlay __ext_gEffectSsOverlayTable[EXT_EFFECT_MAX];
extern u8 gFontOrdering[];
extern Vec3f gZeroVec;

#ifndef __NO_EXT_MACROS__
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
asm ("gPlayState = 0x80212020 - 0x38000;");
asm ("__gfxCtx = gPlayState;");
asm ("gExitParam = gPlayState + 0x11E18;"); // 801EBE38
asm ("osMemSize = 0x80000318");

void uLib_Update(GameState* gameState);
void* memset(void* m, int v, unsigned int s);
f32 fmodf(f32, f32);

#ifdef DEV_BUILD
void Debug_Text(u8 r, u8 g, u8 b, s32 x, s32 y, char* fmt, ...);
void Debug_DmaLog(DmaRequest* req);
void DebugMenu_Update(PlayState* playState);
s32 DebugMenu_CineCamera(Camera* camera, Normal1* norm1, Player* player);
void Profiler_Start(DebugProfiler* profiler);
void Profiler_End(DebugProfiler* profiler);
#else
#define Debug_Text(...)           ((void)0)
#define Debug_DmaLog(...)         ((void)0)
#define DebugMenu_Update(...)     ((void)0)
#define DebugMenu_CineCamera(...) ((void)0)
#define Profiler_Start(...)       ((void)0)
#define Profiler_End(...)         ((void)0)
#endif

#define U32_RGB(x) (u8)(x >> 24), (u8)(x >> 16), (u8)(x >> 8)

#ifndef osLibPrintf
void osLibPrintf(const char* fmt, ...);
#endif

#endif