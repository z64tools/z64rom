#ifndef __ULIB_H__
#define __ULIB_H__

#include <oot_mq_debug/z64hdr.h>

#include <Color/Color.h>
#include <Math/Math.h>
#include <SceneRender/SceneRender.h>
#include <Debug/DebugSys.h>
#include <Gui/Gui.h>

#include "ULib_Macros.h"
#include "ULib_Types.h"

#define EXT_DMA_MAX    3800
#define EXT_ACTOR_MAX  1000
#define EXT_OBJECT_MAX 1000
#define EXT_SCENE_MAX  300
#define EXT_EFFECT_MAX 64

extern DmaEntry __ext_gDmaDataTable[EXT_DMA_MAX];
extern ActorOverlay __ext_gActorOverlayTable[EXT_ACTOR_MAX];
extern RomFile __ext_gObjectTable[EXT_OBJECT_MAX];
extern SceneTableEntry __ext_gSceneTable[EXT_SCENE_MAX];
extern EffectSsOverlay __ext_gEffectSsOverlayTable[EXT_EFFECT_MAX];

#ifndef __NO_EXT_MACROS__
#define gDmaDataTable         __ext_gDmaDataTable
#define gActorOverlayTable    __ext_gActorOverlayTable
#define gObjectTable          __ext_gObjectTable
#define gSceneTable           __ext_gSceneTable
#define gEffectSsOverlayTable __ext_gEffectSsOverlayTable
#endif

extern LibContext gLibCtx;
extern GraphicsContext* __gfxCtx;
asm ("__gfxCtx = 0x80212020");
asm ("gGlobalContext = 0x80212020");

void ULib_Update(GameState* gameState);
void ULib_DmaDebug(DmaRequest* req);
void ULib_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn);

void osLibPrintf(const char* fmt, ...);

#endif