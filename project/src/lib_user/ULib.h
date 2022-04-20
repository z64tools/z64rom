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

extern LibContext gLibCtx;
extern GraphicsContext* __gfxCtx;
asm ("__gfxCtx = 0x80212020");
asm ("gGlobalContext = 0x80212020");

void ULib_Update(GameState* gameState);
void ULib_DmaDebug(DmaRequest* req, DmaEntry* dma);

#endif