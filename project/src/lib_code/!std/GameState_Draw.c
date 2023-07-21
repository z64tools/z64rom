#include <uLib.h>
#include <code/game.h>

/*
   z64ram = 0x800C46EC
   z64rom = 0xB3B88C
 */

void GameState_Draw(GameState* gameState, GraphicsContext* gfxCtx) {
    GraphicsContext* __gfxCtx = gfxCtx;
    Gfx* newDList;
    Gfx* polyOpaP;
    
    newDList = Graph_GfxPlusOne(polyOpaP = POLY_OPA_DISP);
    gSPDisplayList(OVERLAY_DISP++, newDList);
    
    if (R_ENABLE_FB_FILTER == 1)
        GameState_SetFBFilter(&newDList);
    
    sLastButtonPressed = gameState->input[0].press.button | gameState->input[0].cur.button;
    
    gSPEndDisplayList(newDList++);
    Graph_BranchDlist(polyOpaP, newDList);
    POLY_OPA_DISP = newDList;
}
