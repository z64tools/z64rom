#include <uLib.h>

Vtx* Gfx_AllocQuad(PlayState* playState, s16 x, s16 y, s16 width, s16 height, s16 u, s16 v) {
    Vtx* vtx = Graph_Alloc(playState->state.gfxCtx, 4 * sizeof(Vtx));
    
    vtx[0] = gdSPDefVtxC(x,         y + height, 0, 0, 0, 255, 255, 255, 255);
    vtx[1] = gdSPDefVtxC(x + width, y + height, 0, u, 0, 255, 255, 255, 255);
    vtx[2] = gdSPDefVtxC(x,         y,          0, 0, v, 255, 255, 255, 255);
    vtx[3] = gdSPDefVtxC(x + width, y,          0, u, v, 255, 255, 255, 255);
    
    return vtx;
}
