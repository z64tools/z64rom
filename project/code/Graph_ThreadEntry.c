#include <oot_mq_debug/z64hdr.h>

void z64rom_LoadCodeLib(void);

asm("gGameStateOverlayTable = 0x8011F830");

void Graph_ThreadEntry(void* arg0) {
	GraphicsContext gfxCtx;
	GameState* gameState;
	u32 size;
	GameStateOverlay* nextOvl;
	GameStateOverlay* ovl;
	char faultMsg[0x50];
	
	nextOvl = &gGameStateOverlayTable[0];
	
	Graph_Init(&gfxCtx);
	z64rom_LoadCodeLib();
	
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

void z64rom_LoadCodeLib(void) {
	u64* entryPoint = (u64*)0x80600000;
	
	if (entryPoint)
		return;
	
	DmaMgr_SendRequest0(
		0x80600000,
		gDmaDataTable[1].vromStart,
		gDmaDataTable[1].vromEnd - gDmaDataTable[1].vromStart
	);
}