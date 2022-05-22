#include <ULib.h>
#include "vt.h"

void __uLib_DebugMessages(u32 msgID) {
	switch (msgID) {
		case 1: {
			ActorOverlay* overlayEntry;
			u32 i;
			
			osLibPrintf("OverlayTable %d", EXT_ACTOR_MAX);
			osLibPrintf("Start   End     SegStart SegEnd   RamAddr profile  segname\n");
			
			for (i = 0, overlayEntry = &gActorOverlayTable[0]; i < EXT_ACTOR_MAX; i++, overlayEntry++) {
				osSyncPrintf(
					"%08x %08x %08x %08x %08x %08x %s\n",
					overlayEntry->vromStart,
					overlayEntry->vromEnd,
					overlayEntry->vramStart,
					overlayEntry->vramEnd,
					overlayEntry->loadedRamAddr,
					&overlayEntry->initInfo->id,
					overlayEntry->name != NULL ? overlayEntry->name : "?"
				);
			}
			break;
		}
		
		case 2: {
			ActorOverlay* overlayEntry;
			u32 overlaySize;
			s32 i;
			
			FaultDrawer_SetCharPad(-2, 0);
			
			FaultDrawer_Printf("actor_dlftbls %u\n", gMaxActorId);
			FaultDrawer_Printf("No. RamStart- RamEnd cn  Name\n");
			
			for (i = 0, overlayEntry = &gActorOverlayTable[0]; i < gMaxActorId; i++, overlayEntry++) {
				overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
				if (overlayEntry->loadedRamAddr != NULL) {
					FaultDrawer_Printf(
						"%3d %08x-%08x %3d %s\n",
						i,
						overlayEntry->loadedRamAddr,
						(u32)overlayEntry->loadedRamAddr + overlaySize,
						overlayEntry->numLoaded,
						overlayEntry->name != NULL ? overlayEntry->name : ""
					);
				}
			}
		}
	}
}

Actor* __uLib_Actor_Spawn(ActorContext* actorCtx, GlobalContext* globalCtx, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params) {
	Actor* actor;
	ActorInit* actorInit;
	s32 objBankIndex;
	ActorOverlay* overlayEntry;
	u32 temp;
	char* name;
	u32 overlaySize;
	
	overlayEntry = &gActorOverlayTable[actorId];
	ASSERT(actorId < ACTOR_ID_MAX, "profile < ACTOR_DLF_MAX", "../z_actor.c", 6883);
	
	name = overlayEntry->name != NULL ? overlayEntry->name : "";
	overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
	
	if (HREG(20) != 0) {
		// "Actor class addition [%d:%s]"
		osSyncPrintf("アクタークラス追加 [%d:%s]\n", actorId, name);
	}
	
	if (actorCtx->total > ACTOR_NUMBER_MAX) {
		// "Ａｃｔｏｒ set number exceeded"
		osSyncPrintf(VT_COL(YELLOW, BLACK) "Ａｃｔｏｒセット数オーバー\n" VT_RST);
		
		return NULL;
	}
	
	if (overlayEntry->vramStart == 0) {
		if (HREG(20) != 0) {
			osSyncPrintf("オーバーレイではありません\n"); // "Not an overlay"
		}
		
		actorInit = overlayEntry->initInfo;
	} else {
		if (overlayEntry->loadedRamAddr != NULL) {
			if (HREG(20) != 0) {
				osSyncPrintf("既にロードされています\n"); // "Already loaded"
			}
		} else {
			if (overlayEntry->allocType & ALLOCTYPE_ABSOLUTE) {
				ASSERT(overlaySize <= AM_FIELD_SIZE, "actor_segsize <= AM_FIELD_SIZE", "../z_actor.c", 6934);
				
				if (actorCtx->absoluteSpace == NULL) {
					// "AMF: absolute magic field"
					actorCtx->absoluteSpace = ZeldaArena_MallocRDebug(AM_FIELD_SIZE, "AMF:絶対魔法領域", 0);
					if (HREG(20) != 0) {
						// "Absolute magic field reservation - %d bytes reserved"
						osSyncPrintf("絶対魔法領域確保 %d バイト確保\n", AM_FIELD_SIZE);
					}
				}
				
				overlayEntry->loadedRamAddr = actorCtx->absoluteSpace;
			} else if (overlayEntry->allocType & ALLOCTYPE_PERMANENT) {
				overlayEntry->loadedRamAddr = ZeldaArena_MallocRDebug(overlaySize, name, 0);
			} else {
				overlayEntry->loadedRamAddr = ZeldaArena_MallocDebug(overlaySize, name, 0);
			}
			
			if (overlayEntry->loadedRamAddr == NULL) {
				// "Cannot reserve actor program memory"
				osSyncPrintf(VT_COL(RED, WHITE) "Ａｃｔｏｒプログラムメモリが確保できません\n" VT_RST);
				
				return NULL;
			}
			
			Overlay_Load(
				overlayEntry->vromStart,
				overlayEntry->vromEnd,
				overlayEntry->vramStart,
				overlayEntry->vramEnd,
				overlayEntry->loadedRamAddr
			);
			
			osSyncPrintf(VT_FGCOL(GREEN));
			osSyncPrintf(
				"OVL(a):Seg:%08x-%08x Ram:%08x-%08x Off:%08x %s\n",
				overlayEntry->vramStart,
				overlayEntry->vramEnd,
				overlayEntry->loadedRamAddr,
				(u32)overlayEntry->loadedRamAddr + (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart,
				(u32)overlayEntry->vramStart - (u32)overlayEntry->loadedRamAddr,
				name
			);
			osSyncPrintf(VT_RST);
			
			overlayEntry->numLoaded = 0;
		}
		
		actorInit = (void*)(u32)((overlayEntry->initInfo != NULL)
				     ? (void*)((u32)overlayEntry->initInfo -
			(s32)((u32)overlayEntry->vramStart - (u32)overlayEntry->loadedRamAddr))
				     : NULL);
	}
	
	objBankIndex = Object_GetIndex(&globalCtx->objectCtx, actorInit->objectId);
	
	if ((objBankIndex < 0) ||
		((actorInit->category == ACTORCAT_ENEMY) && Flags_GetClear(globalCtx, globalCtx->roomCtx.curRoom.num))) {
		// "No data bank!! <data bank＝%d> (profilep->bank=%d)"
		osSyncPrintf(
			VT_COL(RED, WHITE) "データバンク無し！！<データバンク＝%d>(profilep->bank=%d)\n" VT_RST,
			objBankIndex,
			actorInit->objectId
		);
		Actor_FreeOverlay(overlayEntry);
		
		return NULL;
	}
	
	actor = ZeldaArena_MallocDebug(actorInit->instanceSize, name, 1);
	
	if (actor == NULL) {
		// "Actor class cannot be reserved! %s <size＝%d bytes>"
		osSyncPrintf(
			VT_COL(RED, WHITE) "Ａｃｔｏｒクラス確保できません！ %s <サイズ＝%dバイト>\n",
			VT_RST,
			name,
			actorInit->instanceSize
		);
		Actor_FreeOverlay(overlayEntry);
		
		return NULL;
	}
	
	ASSERT((u8)overlayEntry->numLoaded < 255, "actor_dlftbl->clients < 255", "../z_actor.c", 7031);
	
	overlayEntry->numLoaded++;
	
	if (HREG(20) != 0) {
		// "Actor client No. %d"
		osSyncPrintf("アクタークライアントは %d 個目です\n", overlayEntry->numLoaded);
	}
	
	Lib_MemSet((u8*)actor, actorInit->instanceSize, 0);
	actor->overlayEntry = overlayEntry;
	actor->id = actorInit->id;
	actor->flags = actorInit->flags;
	
	if (actorInit->id == ACTOR_EN_PART) {
		actor->objBankIndex = rotZ;
		rotZ = 0;
	} else {
		actor->objBankIndex = objBankIndex;
	}
	
	actor->init = actorInit->init;
	actor->destroy = actorInit->destroy;
	actor->update = actorInit->update;
	actor->draw = actorInit->draw;
	actor->room = globalCtx->roomCtx.curRoom.num;
	actor->home.pos.x = posX;
	actor->home.pos.y = posY;
	actor->home.pos.z = posZ;
	actor->home.rot.x = rotX;
	actor->home.rot.y = rotY;
	actor->home.rot.z = rotZ;
	actor->params = params;
	
	Actor_AddToCategory(actorCtx, actor, actorInit->category);
	
	temp = gSegments[6];
	Actor_Init(actor, globalCtx);
	gSegments[6] = temp;
	
	return actor;
}

void __uLib_Gameplay_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn) {
	SceneTableEntry* scene = &gSceneTable[sceneNum];
	u32 roomSize;
	
	scene->unk_13 = 0;
	globalCtx->loadedScene = scene;
	globalCtx->sceneNum = sceneNum;
	globalCtx->sceneConfig = scene->config;
	
	globalCtx->sceneSegment = Gameplay_LoadFile(globalCtx, &scene->sceneFile);
	scene->unk_13 = 0;
	
	gSegments[2] = VIRTUAL_TO_PHYSICAL(globalCtx->sceneSegment);
	
	Gameplay_InitScene(globalCtx, spawn);
	roomSize = func_80096FE8(globalCtx, &globalCtx->roomCtx);
	
	osLibPrintf(
		"Scene "
		PRNT_YELW "0x%02X " PRNT_RSET
		"SceneEntry "
		PRNT_YELW "%08X " PRNT_RSET
		"EntryHead "
		PRNT_YELW "%08X " PRNT_RSET
		"Segment "
		PRNT_YELW "%08X ",
		sceneNum,
		scene,
		gSceneTable,
		globalCtx->sceneSegment
	);
	osLibPrintf(
		"Room Size "
		PRNT_YELW "%.1f " PRNT_RSET
		"KB",
		BinToKb(roomSize)
	);
}