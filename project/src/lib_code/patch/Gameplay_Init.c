/*
   z64ram = 0x800BCA64
   z64rom = 0xB33C04
 */

#include <ULib.h>

void __Gameplay_Init(GameState* thisx) {
	GlobalContext* globalCtx = (GlobalContext*)thisx;
	GraphicsContext* gfxCtx = globalCtx->state.gfxCtx;
	u32 zAlloc;
	u32 zAllocAligned;
	size_t zAllocSize;
	Player* player;
	s32 playerStartCamId;
	s32 i;
	u8 tempSetupIndex;
	s32 pad[2];
	
	if (gSaveContext.entranceIndex == -1) {
		gSaveContext.entranceIndex = 0;
		globalCtx->state.running = false;
		SET_NEXT_GAMESTATE(&globalCtx->state, Opening_Init, OpeningContext);
		
		return;
	}
	
	SystemArena_Display();
	GameState_Realloc(&globalCtx->state, 0x3D8000 + 0x3A60);
	KaleidoManager_Init(globalCtx);
	View_Init(&globalCtx->view, gfxCtx);
	Audio_SetExtraFilter(0);
	Quake_Init();
	
	for (i = 0; i < ARRAY_COUNT(globalCtx->cameraPtrs); i++) {
		globalCtx->cameraPtrs[i] = NULL;
	}
	
	Camera_Init(&globalCtx->mainCamera, &globalCtx->view, &globalCtx->colCtx, globalCtx);
	Camera_ChangeStatus(&globalCtx->mainCamera, CAM_STAT_ACTIVE);
	
	for (i = 0; i < 3; i++) {
		Camera_Init(&globalCtx->subCameras[i], &globalCtx->view, &globalCtx->colCtx, globalCtx);
		Camera_ChangeStatus(&globalCtx->subCameras[i], CAM_STAT_UNK100);
	}
	
	globalCtx->cameraPtrs[MAIN_CAM] = &globalCtx->mainCamera;
	globalCtx->cameraPtrs[MAIN_CAM]->uid = 0;
	globalCtx->activeCamera = MAIN_CAM;
	func_8005AC48(&globalCtx->mainCamera, 0xFF);
	Sram_Init(globalCtx, &globalCtx->sramCtx);
	func_80112098(globalCtx);
	Message_Init(globalCtx);
	GameOver_Init(globalCtx);
	SoundSource_InitAll(globalCtx);
	Effect_InitContext(globalCtx);
	EffectSs_InitInfo(globalCtx, 0x55);
	CollisionCheck_InitContext(globalCtx, &globalCtx->colChkCtx);
	AnimationContext_Reset(&globalCtx->animationCtx);
	func_8006450C(globalCtx, &globalCtx->csCtx);
	
	if (gSaveContext.nextCutsceneIndex != 0xFFEF) {
		gSaveContext.cutsceneIndex = gSaveContext.nextCutsceneIndex;
		gSaveContext.nextCutsceneIndex = 0xFFEF;
	}
	
	if (gSaveContext.cutsceneIndex == 0xFFFD) {
		gSaveContext.cutsceneIndex = 0;
	}
	
	if (gSaveContext.nextDayTime != 0xFFFF) {
		gSaveContext.dayTime = gSaveContext.nextDayTime;
		gSaveContext.skyboxTime = gSaveContext.nextDayTime;
	}
	
	if (gSaveContext.dayTime > 0xC000 || gSaveContext.dayTime < 0x4555) {
		gSaveContext.nightFlag = 1;
	} else {
		gSaveContext.nightFlag = 0;
	}
	
	Cutscene_HandleConditionalTriggers(globalCtx);
	
	if (gSaveContext.gameMode != 0 || gSaveContext.cutsceneIndex >= 0xFFF0) {
		gSaveContext.nayrusLoveTimer = 0;
		func_800876C8(globalCtx);
		gSaveContext.sceneSetupIndex = (gSaveContext.cutsceneIndex & 0xF) + 4;
	} else if (!LINK_IS_ADULT && IS_DAY) {
		gSaveContext.sceneSetupIndex = 0;
	} else if (!LINK_IS_ADULT && !IS_DAY) {
		gSaveContext.sceneSetupIndex = 1;
	} else if (LINK_IS_ADULT && IS_DAY) {
		gSaveContext.sceneSetupIndex = 2;
	} else {
		gSaveContext.sceneSetupIndex = 3;
	}
	
	tempSetupIndex = gSaveContext.sceneSetupIndex;
	if ((gEntranceTable[((void)0, gSaveContext.entranceIndex)].scene == SCENE_SPOT00) && !LINK_IS_ADULT &&
		gSaveContext.sceneSetupIndex < 4) {
		if (CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD) && CHECK_QUEST_ITEM(QUEST_GORON_RUBY) &&
			CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE)) {
			gSaveContext.sceneSetupIndex = 1;
		} else {
			gSaveContext.sceneSetupIndex = 0;
		}
	} else if ((gEntranceTable[((void)0, gSaveContext.entranceIndex)].scene == SCENE_SPOT04) && LINK_IS_ADULT &&
		gSaveContext.sceneSetupIndex < 4) {
		gSaveContext.sceneSetupIndex = (gSaveContext.eventChkInf[4] & 0x100) ? 3 : 2;
	}
	
	uLib_Gameplay_SpawnScene(
		globalCtx,
		gEntranceTable[((void)0, gSaveContext.entranceIndex) + ((void)0, gSaveContext.sceneSetupIndex)].scene,
		gEntranceTable[((void)0, gSaveContext.sceneSetupIndex) + ((void)0, gSaveContext.entranceIndex)].spawn
	);
	
	Cutscene_HandleEntranceTriggers(globalCtx);
	KaleidoScopeCall_Init(globalCtx);
	func_801109B0(globalCtx);
	
	if (gSaveContext.nextDayTime != 0xFFFF) {
		if (gSaveContext.nextDayTime == 0x8001) {
			gSaveContext.totalDays++;
			gSaveContext.bgsDayCount++;
			gSaveContext.dogIsLost = true;
			if (Inventory_ReplaceItem(globalCtx, ITEM_WEIRD_EGG, ITEM_CHICKEN) ||
				Inventory_ReplaceItem(globalCtx, ITEM_POCKET_EGG, ITEM_POCKET_CUCCO)) {
				Message_StartTextbox(globalCtx, 0x3066, NULL);
			}
			gSaveContext.nextDayTime = 0xFFFE;
		} else {
			gSaveContext.nextDayTime = 0xFFFD;
		}
	}
	
	SREG(91) = -1;
	R_PAUSE_MENU_MODE = 0;
	PreRender_Init(&globalCtx->pauseBgPreRender);
	PreRender_SetValuesSave(&globalCtx->pauseBgPreRender, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0);
	PreRender_SetValues(&globalCtx->pauseBgPreRender, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	gTrnsnUnkState = 0;
	globalCtx->transitionMode = 0;
	FrameAdvance_Init(&globalCtx->frameAdvCtx);
	Rand_Seed((u32)osGetTime());
	Matrix_Init(&globalCtx->state);
	globalCtx->state.main = Gameplay_Main;
	globalCtx->state.destroy = Gameplay_Destroy;
	globalCtx->sceneLoadFlag = -0x14;
	globalCtx->unk_11E16 = 0xFF;
	globalCtx->unk_11E18 = 0;
	globalCtx->unk_11DE9 = 0;
	
	if (gSaveContext.gameMode != 1) {
		if (gSaveContext.nextTransition == 0xFF) {
			globalCtx->fadeTransition =
				(gEntranceTable[((void)0, gSaveContext.entranceIndex) + tempSetupIndex].field >> 7) & 0x7F; // Fade In
		} else {
			globalCtx->fadeTransition = gSaveContext.nextTransition;
			gSaveContext.nextTransition = 0xFF;
		}
	} else {
		globalCtx->fadeTransition = 6;
	}
	
	ShrinkWindow_Init();
	TransitionFade_Init(&globalCtx->transitionFade);
	TransitionFade_SetType(&globalCtx->transitionFade, 3);
	TransitionFade_SetColor(&globalCtx->transitionFade, RGBA8(160, 160, 160, 255));
	TransitionFade_Start(&globalCtx->transitionFade);
	VisMono_Init(&D_80161498);
	D_801614B0.a = 0;
	Flags_UnsetAllEnv(globalCtx);
	
	zAllocSize = THA_GetSize(&globalCtx->state.tha);
	zAlloc = (u32)GameState_Alloc(&globalCtx->state, zAllocSize, "../z_play.c", 2918);
	zAllocAligned = (zAlloc + 8) & ~0xF;
	ZeldaArena_Init((void*)zAllocAligned, zAllocSize - zAllocAligned + zAlloc);
	
	Fault_AddClient(&D_801614B8, ZeldaArena_Display, NULL, NULL);
	func_800304DC(globalCtx, &globalCtx->actorCtx, globalCtx->linkActorEntry);
	
	while (!func_800973FC(globalCtx, &globalCtx->roomCtx));
	
	player = GET_PLAYER(globalCtx);
	Camera_InitPlayerSettings(&globalCtx->mainCamera, player);
	Camera_ChangeMode(&globalCtx->mainCamera, CAM_MODE_NORMAL);
	
	playerStartCamId = player->actor.params & 0xFF;
	if (playerStartCamId != 0xFF) {
		Camera_ChangeDataIdx(&globalCtx->mainCamera, playerStartCamId);
	}
	
	if (YREG(15) == 32) {
		globalCtx->unk_1242B = 2;
	} else if (YREG(15) == 16) {
		globalCtx->unk_1242B = 1;
	} else {
		globalCtx->unk_1242B = 0;
	}
	
	Interface_SetSceneRestrictions(globalCtx);
	Environment_PlaySceneSequence(globalCtx);
	gSaveContext.seqId = globalCtx->sequenceCtx.seqId;
	gSaveContext.natureAmbienceId = globalCtx->sequenceCtx.natureAmbienceId;
	func_8002DF18(globalCtx, GET_PLAYER(globalCtx));
	AnimationContext_Update(globalCtx, &globalCtx->animationCtx);
	gSaveContext.respawnFlag = 0;
	
	if (dREG(95) != 0) {
		D_8012D1F0 = D_801614D0;
		osSyncPrintf("\nkawauso_data=[%x]", D_8012D1F0);
		DmaMgr_DmaRomToRam(0x03FEB000, (u32)D_8012D1F0, sizeof(D_801614D0));
	}
}