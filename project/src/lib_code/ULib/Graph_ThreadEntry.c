#include <ULib.h>

extern void* sSceneDrawHandlers[53];
asm ("sSceneDrawHandlers = 0x8012A3A4");
asm ("gGameStateOverlayTable = 0x8011F830");

void Graph_ThreadEntry(void* arg0) {
	GraphicsContext gfxCtx;
	GameState* gameState;
	u32 size;
	GameStateOverlay* nextOvl;
	GameStateOverlay* ovl;
	char faultMsg[0x50];
	
	nextOvl = &gGameStateOverlayTable[0];
	
	Graph_Init(&gfxCtx);
	{
		/**
		 * DmaRequest Dma Entry 1, which will hold the
		 * global_lib code. This way it can be accessed
		 * thourgh actors or other code
		 */
		u32* entryPoint = (u32*)0x80600000;
		
		if (entryPoint[0] != 0)
			return;
		
		DmaMgr_SendRequest0(
			0x80600000,
			gDmaDataTable[1].vromStart,
			gDmaDataTable[1].vromEnd - gDmaDataTable[1].vromStart
		);
		sSceneDrawHandlers[4] = SceneAnim_Update;
	}
	
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