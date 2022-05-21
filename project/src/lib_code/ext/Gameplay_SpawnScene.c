#include <ULib.h>

/*
   z64ram = 0x800C0008
   z64rom = 0xB371A8
 */

void Gameplay_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn) {
	SceneTableEntry* scene = &gExtSceneTable[sceneNum];
	
	scene = &gSceneTable[sceneNum];
	
#ifdef DEV_BUILD
	ULib_SpawnScene(globalCtx, sceneNum, spawn);
#endif
	
	scene->unk_13 = 0;
	globalCtx->loadedScene = scene;
	globalCtx->sceneNum = sceneNum;
	globalCtx->sceneConfig = scene->config;
	
	globalCtx->sceneSegment = Gameplay_LoadFile(globalCtx, &scene->sceneFile);
	scene->unk_13 = 0;
	
	gSegments[2] = VIRTUAL_TO_PHYSICAL(globalCtx->sceneSegment);
	
	Gameplay_InitScene(globalCtx, spawn);
}