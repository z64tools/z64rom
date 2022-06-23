#ifndef __ULIB_H__
#define __ULIB_H__

#include <oot_mq_debug/z64hdr.h>

#include <library/Library.h>
#include <vanilla/Vanilla.h>

#include "uLib_macros.h"
#include "uLib_types.h"

#define EXT_DMA_MAX    3800
#define EXT_ACTOR_MAX  1000
#define EXT_OBJECT_MAX 1000
#define EXT_SCENE_MAX  256
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

extern PlayState gPlayState;
extern LibContext gLibCtx;
extern GraphicsContext* __gfxCtx;
asm ("__gfxCtx = 0x80212020 - 0x38000;");
asm ("gPlayState = 0x80212020 - 0x38000;");

void uLib_Update(GameState* gameState);
void* memset(void* m, int v, unsigned int s);

#define U32_RGB(x) (u8)(x >> 24), (u8)(x >> 16), (u8)(x >> 8)

#ifndef osLibPrintf
void osLibPrintf(const char* fmt, ...);
#endif

#endif