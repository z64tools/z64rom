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

extern DmaEntry gExtDmaTable[EXT_DMA_MAX];
extern ActorOverlay gExtActorTable[EXT_ACTOR_MAX];
extern RomFile gExtObjectTable[EXT_OBJECT_MAX];
extern SceneTableEntry gExtSceneTable[EXT_SCENE_MAX];
extern EffectSsOverlay gExtEffectTable[EXT_EFFECT_MAX];

extern LibContext gLibCtx;
extern GraphicsContext* __gfxCtx;
asm ("__gfxCtx = 0x80212020");
asm ("gGlobalContext = 0x80212020");

void ULib_Update(GameState* gameState);
void ULib_DmaDebug(DmaRequest* req, DmaEntry* dma);
void ULib_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn);

#endif