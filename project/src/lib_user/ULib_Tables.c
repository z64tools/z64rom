#include <ULib.h>
#include "vt.h"

LibContext gLibCtx = {
	.state = {
		.vanillaOsPrintf = false,
	},
	.__ctxInitValue = 0xDEADBEEF,
};

__attribute__((no_reorder, aligned(0x10))) DmaEntry __ext_gDmaDataTable[EXT_DMA_MAX] = { 1 };
__attribute__((no_reorder, aligned(0x10))) ActorOverlay __ext_gActorOverlayTable[EXT_ACTOR_MAX] = { 1 };
__attribute__((no_reorder, aligned(0x10))) RomFile __ext_gObjectTable[EXT_OBJECT_MAX] = { 1 };
__attribute__((no_reorder, aligned(0x10))) SceneTableEntry __ext_gSceneTable[EXT_SCENE_MAX] = { 1 };
__attribute__((no_reorder, aligned(0x10))) EffectSsOverlay __ext_gEffectSsOverlayTable[EXT_EFFECT_MAX] = { 1 };
__attribute__((no_reorder, aligned(0x10))) u32 ____padding[100] = { 1 };