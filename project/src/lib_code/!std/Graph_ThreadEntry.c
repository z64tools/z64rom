#include <uLib.h>

/*
   z64ram = 0x800C6844
   z64rom = 0xB3D9E4
 */

extern void* sSceneDrawHandlers[53];
asm ("sSceneDrawHandlers = 0x8012A3A4");
asm ("gGameStateOverlayTable = 0x8011F830");

void Graph_ThreadEntry(void* arg0) {
    GraphicsContext gfxCtx;
    GameState* gameState;
    u32 size;
    GameStateOverlay* nextOvl;
    GameStateOverlay* ovl;
    
    nextOvl = &gGameStateOverlayTable[0];
    
    Graph_Init(&gfxCtx);
    
    while (nextOvl) {
        ovl = nextOvl;
        Overlay_LoadGameState(ovl);
        
        size = ovl->instanceSize;
        gameState = SystemArena_MallocDebug(size, "", 0);
        
        if (!gameState) {
            Fault_AddHungupAndCrashImpl("gameStateNULL", "");
        }
        GameState_Init(gameState, ovl->init, &gfxCtx);
        
        while (GameState_IsRunning(gameState)) {
            Graph_Update(&gfxCtx, gameState);
        }
        
        nextOvl = Graph_GetNextGameState(gameState);
        GameState_Destroy(gameState);
        SystemArena_FreeDebug(gameState, "", 0);
        Overlay_FreeGameState(ovl);
    }
    Graph_Destroy(&gfxCtx);
}
