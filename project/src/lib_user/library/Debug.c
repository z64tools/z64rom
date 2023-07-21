#include <uLib.h>
#include "code/gfxprint.h"

u32 __mib_DebugC;

#ifdef DEV_BUILD

void Debug_Text(u8 r, u8 g, u8 b, s32 x, s32 y, char* fmt, ...) {
    va_list args;
    GfxPrint debTex;
    Gfx* polyOpa = POLY_OPA_DISP;
    Gfx* gfx = Graph_GfxPlusOne(polyOpa);
    
    va_start(args, fmt);
    gSPDisplayList(OVERLAY_DISP++, gfx);
    GfxPrint_Init(&debTex);
    GfxPrint_Open(&debTex, gfx);
    GfxPrint_SetColor(&debTex, r, g, b, 255);
    GfxPrint_SetPos(&debTex, x, y);
    GfxPrint_VPrintf(&debTex, fmt, args);
    gfx = GfxPrint_Close(&debTex);
    GfxPrint_Destroy(&debTex);
    
    gSPEndDisplayList(gfx++);
    Graph_BranchDlist(polyOpa, gfx);
    POLY_OPA_DISP = gfx;
    
    va_end(args);
}

void Debug_DmaLog(DmaRequest* req) {
    u32 id = 0;
    DmaEntry* iter = gDmaDataTable;
    
    if (gLibCtx.state.dmaLog == false)
        return;
    
    while (iter->vromEnd) {
        if (req->vromAddr >= iter->vromStart && req->vromAddr < iter->vromEnd) {
            break;
        }
        
        id++;
        iter++;
    }
    
    // No Messages || No LinkAnim
    if (id == 0x14 || id == 0x6 || id == 0x11 || id == 0x12 || id >= EXT_DMA_MAX)
        return;
    
    osLibPrintf("Dma Request:");
    
    #if 0 // Crashes on some names
    if (req->filename)
        osLibPrintf("" PRNT_GRAY "[" PRNT_REDD "%s" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]", req->filename, req->line);
    
    else
        osLibPrintf("" PRNT_GRAY " [" PRNT_REDD "null" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]", req->line);
    #endif
    
    osLibPrintf(
        "vrom " PRNT_BLUE "%08X - %08X & %08X - %08X" PRNT_RSET " size: " PRNT_BLUE "%08X " PRNT_RSET "[ " PRNT_REDD "ID 0x%04X" PRNT_RSET " ]",
        iter->vromStart,
        iter->vromEnd,
        iter->romStart,
        iter->romEnd,
        req->size,
        id
    );
    osLibPrintf("ram:  " PRNT_PRPL "%08X", req->dramAddr);
}

#endif