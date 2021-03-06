#include <uLib.h>
#include "vt.h"
#include "code/z_actor.h"
#include "code/z_effect_soft_sprite.h"

Actor* Overlay_ActorSpawn(ActorContext* actorCtx, PlayState* playState, s16 actorId, f32 posX, f32 posY, f32 posZ, s16 rotX, s16 rotY, s16 rotZ, s16 params) {
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
	
	objBankIndex = Object_GetIndex(&playState->objectCtx, actorInit->objectId);
	
	if ((objBankIndex < 0) ||
		((actorInit->category == ACTORCAT_ENEMY) && Flags_GetClear(playState, playState->roomCtx.curRoom.num))) {
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
	actor->room = playState->roomCtx.curRoom.num;
	actor->home.pos.x = posX;
	actor->home.pos.y = posY;
	actor->home.pos.z = posZ;
	actor->home.rot.x = rotX;
	actor->home.rot.y = rotY;
	actor->home.rot.z = rotZ;
	actor->params = params;
	
	Actor_AddToCategory(actorCtx, actor, actorInit->category);
	
	temp = gSegments[6];
	Actor_Init(actor, playState);
	gSegments[6] = temp;
	
	return actor;
}

void Overlay_EffectSpawn(PlayState* playState, s32 type, s32 priority, void* initParams) {
	s32 index;
	u32 overlaySize;
	EffectSsOverlay* overlayEntry;
	EffectSsInit* initInfo;
	
	overlayEntry = &gEffectSsOverlayTable[type];
	
	Assert(type < EXT_EFFECT_MAX);
	
	if (EffectSs_FindSlot(priority, &index) != 0) {
		osLibPrintf(osInfo("Break"));
		osLibPrintf("Could not find free slot!");
		
		return;
	}
	
	sEffectSsInfo.searchStartIndex = index + 1;
	overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
	
	if (overlayEntry->vramStart == NULL) {
		osLibPrintf("Not an overlay");
		initInfo = overlayEntry->initInfo;
	} else {
		if (overlayEntry->loadedRamAddr == NULL) {
			overlayEntry->loadedRamAddr = ZeldaArena_MallocRDebug(overlaySize, __FUNCTION__, __LINE__);
			
			if (overlayEntry->loadedRamAddr == NULL) {
				osLibPrintf(osInfo("Break"));
				osLibPrintf("The memory of [" PRNT_BLUE "%.2f" PRNT_RSET "kB] byte cannot be secured. Therefore, the program cannot be loaded.", BinToKb(overlaySize));
				osLibPrintf("What a dangerous situation! Naturally, effects will not produced either.");
				
				return;
			}
			
			Overlay_Load(
				overlayEntry->vromStart,
				overlayEntry->vromEnd,
				overlayEntry->vramStart,
				overlayEntry->vramEnd,
				overlayEntry->loadedRamAddr
			);
		}
		
		initInfo = (void*)(u32)((overlayEntry->initInfo != NULL)
				    ? (void*)((u32)overlayEntry->initInfo -
			(s32)((u32)overlayEntry->vramStart - (u32)overlayEntry->loadedRamAddr))
				    : NULL);
	}
	
	if (initInfo->init == NULL) {
		// "Effects have already been loaded, but the constructor is NULL so the addition will not occur.
		// Please fix this. (Waste of memory) %08x %d"
		osLibPrintf(osInfo("Break"));
		osLibPrintf("Effects have already been loaded, but the constructor is NULL so the addition will not occur.");
		osLibPrintf("Please fix this. InitInfo: %08X ID: %d", initInfo, type);
		
		return;
	}
	
	// Delete the previous effect in the slot, in case the slot wasn't free
	EffectSs_Delete(&sEffectSsInfo.table[index]);
	
	sEffectSsInfo.table[index].type = type;
	sEffectSsInfo.table[index].priority = priority;
	
	if (initInfo->init(playState, index, &sEffectSsInfo.table[index], initParams) == 0) {
		osLibPrintf(osInfo("Break"));
		osLibPrintf("Construction failed for some reason. The constructor returned an error");
		osLibPrintf("Ceasing effect addition...");
		EffectSs_Reset(&sEffectSsInfo.table[index]);
	}
}