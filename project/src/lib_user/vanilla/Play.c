#include <uLib.h>

void Play_SpawnScene(GlobalContext* globalCtx, s32 sceneNum, s32 spawn) {
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

void Play_Init(GlobalContext* globalCtx) {
	GraphicsContext* gfxCtx = globalCtx->state.gfxCtx;
	u32 zAlloc;
	u32 zAllocAligned;
	size_t zAllocSize;
	Player* player;
	s32 playerStartCamId;
	s32 i;
	u8 tempSetupIndex;
	
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
	
	Play_SpawnScene(
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

void Play_Draw(GlobalContext* globalCtx) {
	GraphicsContext* gfxCtx = globalCtx->state.gfxCtx;
	Lights* sp228;
	Vec3f sp21C;
	
	POLY_OPA_DISP = Gameplay_SetFog(globalCtx, POLY_OPA_DISP);
	POLY_XLU_DISP = Gameplay_SetFog(globalCtx, POLY_XLU_DISP);
	
	View_SetPerspective(&globalCtx->view, globalCtx->view.fovy, globalCtx->view.zNear, globalCtx->lightCtx.fogFar);
	View_Apply(&globalCtx->view, VIEW_ALL);
	
	// The billboard matrix temporarily stores the viewing matrix
	Matrix_MtxToMtxF(&globalCtx->view.viewing, &globalCtx->billboardMtxF);
	Matrix_MtxToMtxF(&globalCtx->view.projection, &globalCtx->viewProjectionMtxF);
	Matrix_Mult(&globalCtx->viewProjectionMtxF, MTXMODE_NEW);
	// The billboard is still a viewing matrix at this stage
	Matrix_Mult(&globalCtx->billboardMtxF, MTXMODE_APPLY);
	Matrix_Get(&globalCtx->viewProjectionMtxF);
	globalCtx->billboardMtxF.mf[0][3] = globalCtx->billboardMtxF.mf[1][3] = globalCtx->billboardMtxF.mf[2][3] =
		globalCtx->billboardMtxF.mf[3][0] = globalCtx->billboardMtxF.mf[3][1] = globalCtx->billboardMtxF.mf[3][2] =
		0.0f;
	// This transpose is where the viewing matrix is properly converted into a billboard matrix
	Matrix_Transpose(&globalCtx->billboardMtxF);
	globalCtx->billboardMtx = Matrix_MtxFToMtx(
		Matrix_CheckFloats(&globalCtx->billboardMtxF, "../z_play.c", 4005),
		Graph_Alloc(gfxCtx, sizeof(Mtx))
	);
	
	gSPSegment(POLY_OPA_DISP++, 0x01, globalCtx->billboardMtx);
	
	if ((HREG(80) != 10) || (HREG(92) != 0)) {
		Gfx* gfxP;
		Gfx* sp1CC = POLY_OPA_DISP;
		
		gfxP = Graph_GfxPlusOne(sp1CC);
		gSPDisplayList(OVERLAY_DISP++, gfxP);
		
		if ((globalCtx->transitionMode == TRANS_MODE_INSTANCE_RUNNING) ||
			(globalCtx->transitionMode == TRANS_MODE_INSTANCE_WAIT) ||
			(globalCtx->transitionCtx.transitionType >= 56)) {
			View view;
			
			View_Init(&view, gfxCtx);
			view.flags = VIEW_VIEWPORT | VIEW_PROJECTION_ORTHO;
			
			SET_FULLSCREEN_VIEWPORT(&view);
			
			View_ApplyTo(&view, VIEW_ALL, &gfxP);
			globalCtx->transitionCtx.draw(globalCtx->transitionCtx.data, &gfxP);
		}
		
		TransitionFade_Draw(&globalCtx->transitionFade, &gfxP);
		
		if (D_801614B0.a > 0) {
			D_80161498.primColor.rgba = D_801614B0.rgba;
			VisMono_Draw(&D_80161498, &gfxP);
		}
		
		gSPEndDisplayList(gfxP++);
		Graph_BranchDlist(sp1CC, gfxP);
		POLY_OPA_DISP = gfxP;
	}
	
	if (gTrnsnUnkState == 3) {
		Gfx* sp88 = POLY_OPA_DISP;
		
		TransitionUnk_Draw(&sTrnsnUnk, &sp88);
		POLY_OPA_DISP = sp88;
		goto Gameplay_Draw_DrawOverlayElements;
	} else {
		PreRender_SetValues(
			&globalCtx->pauseBgPreRender,
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			gfxCtx->curFrameBuffer,
			gZBuffer
		);
		
		if (R_PAUSE_MENU_MODE == 2) {
			MsgEvent_SendNullTask();
			PreRender_Calc(&globalCtx->pauseBgPreRender);
			R_PAUSE_MENU_MODE = 3;
		} else if (R_PAUSE_MENU_MODE >= 4) {
			R_PAUSE_MENU_MODE = 0;
		}
		
		if (R_PAUSE_MENU_MODE == 3) {
			Gfx* sp84 = POLY_OPA_DISP;
			
			func_800C24BC(&globalCtx->pauseBgPreRender, &sp84);
			POLY_OPA_DISP = sp84;
			goto Gameplay_Draw_DrawOverlayElements;
		} else {
			s32 sp80 = 0;
			
			if ((HREG(80) != 10) || (HREG(83) != 0)) {
				if (globalCtx->skyboxId && (globalCtx->skyboxId != SKYBOX_UNSET_1D) &&
					!globalCtx->envCtx.skyboxDisabled) {
					if ((globalCtx->skyboxId == SKYBOX_NORMAL_SKY) ||
						(globalCtx->skyboxId == SKYBOX_CUTSCENE_MAP)) {
						Environment_UpdateSkybox(globalCtx->skyboxId, &globalCtx->envCtx, &globalCtx->skyboxCtx);
						SkyboxDraw_Draw(
							&globalCtx->skyboxCtx,
							gfxCtx,
							globalCtx->skyboxId,
							globalCtx->envCtx.skyboxBlend,
							globalCtx->view.eye.x,
							globalCtx->view.eye.y,
							globalCtx->view.eye.z
						);
					} else if (globalCtx->skyboxCtx.unk_140 == 0) {
						SkyboxDraw_Draw(
							&globalCtx->skyboxCtx,
							gfxCtx,
							globalCtx->skyboxId,
							0,
							globalCtx->view.eye.x,
							globalCtx->view.eye.y,
							globalCtx->view.eye.z
						);
					}
				}
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 2)) {
				if (!globalCtx->envCtx.sunMoonDisabled) {
					Environment_DrawSunAndMoon(globalCtx);
				}
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 1)) {
				Environment_DrawSkyboxFilters(globalCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 4)) {
				Environment_UpdateLightningStrike(globalCtx);
				Environment_DrawLightning(globalCtx, 0);
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 8)) {
				sp228 = LightContext_NewLights(&globalCtx->lightCtx, gfxCtx);
				Lights_BindAll(sp228, globalCtx->lightCtx.listHead, NULL);
				Lights_Draw(sp228, gfxCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(84) != 0)) {
				if (VREG(94) == 0) {
					if (HREG(80) != 10) {
						sp80 = 3;
					} else {
						sp80 = HREG(84);
					}
					Scene_Draw(globalCtx);
					Room_Draw(globalCtx, &globalCtx->roomCtx.curRoom, sp80 & 3);
					Room_Draw(globalCtx, &globalCtx->roomCtx.prevRoom, sp80 & 3);
				}
			}
			
			if ((HREG(80) != 10) || (HREG(83) != 0)) {
				if ((globalCtx->skyboxCtx.unk_140 != 0) &&
					(GET_ACTIVE_CAM(globalCtx)->setting != CAM_SET_PREREND_FIXED)) {
					Vec3f sp74;
					
					Camera_GetSkyboxOffset(&sp74, GET_ACTIVE_CAM(globalCtx));
					SkyboxDraw_Draw(
						&globalCtx->skyboxCtx,
						gfxCtx,
						globalCtx->skyboxId,
						0,
						globalCtx->view.eye.x + sp74.x,
						globalCtx->view.eye.y + sp74.y,
						globalCtx->view.eye.z + sp74.z
					);
				}
			}
			
			if (globalCtx->envCtx.unk_EE[1] != 0) {
				Environment_DrawRain(globalCtx, &globalCtx->view, gfxCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(84) != 0)) {
				Environment_FillScreen(gfxCtx, 0, 0, 0, globalCtx->unk_11E18, FILL_SCREEN_OPA);
			}
			
			if ((HREG(80) != 10) || (HREG(85) != 0)) {
				func_800315AC(globalCtx, &globalCtx->actorCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(86) != 0)) {
				if (!globalCtx->envCtx.sunMoonDisabled) {
					sp21C.x = globalCtx->view.eye.x + globalCtx->envCtx.sunPos.x;
					sp21C.y = globalCtx->view.eye.y + globalCtx->envCtx.sunPos.y;
					sp21C.z = globalCtx->view.eye.z + globalCtx->envCtx.sunPos.z;
					Environment_DrawSunLensFlare(globalCtx, &globalCtx->envCtx, &globalCtx->view, gfxCtx, sp21C, 0);
				}
				Environment_DrawCustomLensFlare(globalCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(87) != 0)) {
				if (MREG(64) != 0) {
					Environment_FillScreen(
						gfxCtx,
						MREG(65),
						MREG(66),
						MREG(67),
						MREG(68),
						FILL_SCREEN_OPA | FILL_SCREEN_XLU
					);
				}
				
				switch (globalCtx->envCtx.fillScreen) {
					case 1:
						Environment_FillScreen(
							gfxCtx,
							globalCtx->envCtx.screenFillColor[0],
							globalCtx->envCtx.screenFillColor[1],
							globalCtx->envCtx.screenFillColor[2],
							globalCtx->envCtx.screenFillColor[3],
							FILL_SCREEN_OPA | FILL_SCREEN_XLU
						);
						break;
					default:
						break;
				}
			}
			
			if ((HREG(80) != 10) || (HREG(88) != 0)) {
				if (globalCtx->envCtx.sandstormState != 0) {
					Environment_DrawSandstorm(globalCtx, globalCtx->envCtx.sandstormState);
				}
			}
			
			if ((R_PAUSE_MENU_MODE == 1) || (gTrnsnUnkState == 1)) {
				Gfx* sp70 = OVERLAY_DISP;
				
				globalCtx->pauseBgPreRender.fbuf = gfxCtx->curFrameBuffer;
				globalCtx->pauseBgPreRender.fbufSave = (u16*)gZBuffer;
				func_800C1F20(&globalCtx->pauseBgPreRender, &sp70);
				if (R_PAUSE_MENU_MODE == 1) {
					globalCtx->pauseBgPreRender.cvgSave = (u8*)gfxCtx->curFrameBuffer;
					func_800C20B4(&globalCtx->pauseBgPreRender, &sp70);
					R_PAUSE_MENU_MODE = 2;
				} else {
					gTrnsnUnkState = 2;
				}
				OVERLAY_DISP = sp70;
				globalCtx->unk_121C7 = 2;
				SREG(33) |= 1;
			} else {
Gameplay_Draw_DrawOverlayElements:
				if ((HREG(80) != 10) || (HREG(89) != 0)) {
					Gameplay_DrawOverlayElements(globalCtx);
				}
			}
		}
	}
}

void Play_Update(GlobalContext* globalCtx) {
	s32 sp80 = 0;
	Input* input;
	
	input = globalCtx->state.input;
	
	gSegments[4] = VIRTUAL_TO_PHYSICAL(globalCtx->objectCtx.status[globalCtx->objectCtx.mainKeepIndex].segment);
	gSegments[5] = VIRTUAL_TO_PHYSICAL(globalCtx->objectCtx.status[globalCtx->objectCtx.subKeepIndex].segment);
	gSegments[2] = VIRTUAL_TO_PHYSICAL(globalCtx->sceneSegment);
	
	if (FrameAdvance_Update(&globalCtx->frameAdvCtx, &input[1])) {
		if ((globalCtx->transitionMode == TRANS_MODE_OFF) && (globalCtx->sceneLoadFlag != TRANS_TRIGGER_OFF)) {
			globalCtx->transitionMode = TRANS_MODE_SETUP;
		}
		
		if (gTrnsnUnkState != 0) {
			switch (gTrnsnUnkState) {
				case 2:
					if (TransitionUnk_Init(&sTrnsnUnk, 10, 7) == NULL) {
						gTrnsnUnkState = 0;
					} else {
						sTrnsnUnk.zBuffer = (u16*)gZBuffer;
						gTrnsnUnkState = 3;
						R_UPDATE_RATE = 1;
					}
					break;
				case 3:
					func_800B23E8(&sTrnsnUnk);
					break;
			}
		}
		
		if (globalCtx->transitionMode) {
			switch (globalCtx->transitionMode) {
				case TRANS_MODE_SETUP:
					if (globalCtx->sceneLoadFlag != TRANS_TRIGGER_END) {
						s16 sceneSetupIndex = 0;
						
						Interface_ChangeAlpha(1);
						
						if (gSaveContext.cutsceneIndex >= 0xFFF0) {
							sceneSetupIndex = (gSaveContext.cutsceneIndex & 0xF) + 4;
						}
						
						// fade out bgm if "continue bgm" flag is not set
						if (!(gEntranceTable[globalCtx->nextEntranceIndex + sceneSetupIndex].field & 0x8000)) {
							if ((globalCtx->fadeTransition < TRANS_TYPE_MAX) &&
								!Environment_IsForcedSequenceDisabled()) {
								func_800F6964(0x14);
								gSaveContext.seqId = (u8)NA_BGM_DISABLED;
								gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
							}
						}
					}
					
					if (!R_TRANS_DBG_ENABLED)
						Gameplay_SetupTransition(globalCtx, globalCtx->fadeTransition);
					else
						Gameplay_SetupTransition(globalCtx, R_TRANS_DBG_TYPE);
					
					if (globalCtx->transitionMode >= TRANS_MODE_FILL_WHITE_INIT) {
						// non-instance modes break out of this switch
						break;
					}
				// fallthrough
				case TRANS_MODE_INSTANCE_INIT:
					globalCtx->transitionCtx.init(globalCtx->transitionCtx.data);
					
					// circle types
					if ((globalCtx->transitionCtx.transitionType >> 5) == 1) {
						globalCtx->transitionCtx.setType(
							globalCtx->transitionCtx.data,
							globalCtx->transitionCtx.transitionType | TC_SET_PARAMS
						);
					}
					
					gSaveContext.unk_1419 = 14;
					
					if ((globalCtx->transitionCtx.transitionType == TRANS_TYPE_WIPE_FAST) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FILL_WHITE2)) {
						//! @bug TRANS_TYPE_FILL_WHITE2 will never reach this code.
						//! It is a non-instance type transition which doesn't run this case.
						gSaveContext.unk_1419 = 28;
					}
					
					gSaveContext.fadeDuration = 60;
					
					if ((globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_BLACK_FAST) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_FAST)) {
						gSaveContext.fadeDuration = 20;
					} else if ((globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_BLACK_SLOW) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_SLOW)) {
						gSaveContext.fadeDuration = 150;
					} else if (globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_INSTANT) {
						gSaveContext.fadeDuration = 2;
					}
					
					if ((globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_FAST) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_SLOW) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_CS_DELAYED) ||
						(globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_INSTANT)) {
						globalCtx->transitionCtx.setColor(
							globalCtx->transitionCtx.data,
							RGBA8(160, 160, 160, 255)
						);
						if (globalCtx->transitionCtx.setEnvColor != NULL) {
							globalCtx->transitionCtx.setEnvColor(
								globalCtx->transitionCtx.data,
								RGBA8(160, 160, 160, 255)
							);
						}
					} else if (globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_GREEN) {
						globalCtx->transitionCtx.setColor(
							globalCtx->transitionCtx.data,
							RGBA8(140, 140, 100, 255)
						);
						
						if (globalCtx->transitionCtx.setEnvColor != NULL) {
							globalCtx->transitionCtx.setEnvColor(
								globalCtx->transitionCtx.data,
								RGBA8(140, 140, 100, 255)
							);
						}
					} else if (globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_BLUE) {
						globalCtx->transitionCtx.setColor(
							globalCtx->transitionCtx.data,
							RGBA8(70, 100, 110, 255)
						);
						
						if (globalCtx->transitionCtx.setEnvColor != NULL) {
							globalCtx->transitionCtx.setEnvColor(
								globalCtx->transitionCtx.data,
								RGBA8(70, 100, 110, 255)
							);
						}
					} else {
						globalCtx->transitionCtx.setColor(globalCtx->transitionCtx.data, RGBA8(0, 0, 0, 0));
						
						if (globalCtx->transitionCtx.setEnvColor != NULL) {
							globalCtx->transitionCtx.setEnvColor(
								globalCtx->transitionCtx.data,
								RGBA8(0, 0, 0, 0)
							);
						}
					}
					
					if (globalCtx->sceneLoadFlag == TRANS_TRIGGER_END) {
						globalCtx->transitionCtx.setType(globalCtx->transitionCtx.data, 1);
					} else {
						globalCtx->transitionCtx.setType(globalCtx->transitionCtx.data, 2);
					}
					
					globalCtx->transitionCtx.start(globalCtx->transitionCtx.data);
					
					if (globalCtx->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_CS_DELAYED) {
						globalCtx->transitionMode = TRANS_MODE_INSTANCE_WAIT;
					} else {
						globalCtx->transitionMode = TRANS_MODE_INSTANCE_RUNNING;
					}
					break;
					
				case TRANS_MODE_INSTANCE_RUNNING:
					if (globalCtx->transitionCtx.isDone(globalCtx->transitionCtx.data)) {
						if (globalCtx->transitionCtx.transitionType >= TRANS_TYPE_MAX) {
							if (globalCtx->sceneLoadFlag == TRANS_TRIGGER_END) {
								globalCtx->transitionCtx.destroy(globalCtx->transitionCtx.data);
								func_800BC88C(globalCtx);
								globalCtx->transitionMode = TRANS_MODE_OFF;
							}
						} else if (globalCtx->sceneLoadFlag != TRANS_TRIGGER_END) {
							globalCtx->state.running = false;
							
							if (gSaveContext.gameMode != 2) {
								SET_NEXT_GAMESTATE(&globalCtx->state, Gameplay_Init, GlobalContext);
								gSaveContext.entranceIndex = globalCtx->nextEntranceIndex;
								
								if (gSaveContext.minigameState == 1) {
									gSaveContext.minigameState = 3;
								}
							} else {
								SET_NEXT_GAMESTATE(&globalCtx->state, FileChoose_Init, FileChooseContext);
							}
						} else {
							globalCtx->transitionCtx.destroy(globalCtx->transitionCtx.data);
							func_800BC88C(globalCtx);
							globalCtx->transitionMode = TRANS_MODE_OFF;
							
							if (gTrnsnUnkState == 3) {
								TransitionUnk_Destroy(&sTrnsnUnk);
								gTrnsnUnkState = 0;
								R_UPDATE_RATE = 3;
							}
						}
						
						globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
					} else {
						globalCtx->transitionCtx.update(globalCtx->transitionCtx.data, R_UPDATE_RATE);
					}
					break;
			}
			
			// update non-instance transitions
			switch (globalCtx->transitionMode) {
				case TRANS_MODE_FILL_WHITE_INIT:
					D_801614C8 = 0;
					globalCtx->envCtx.fillScreen = true;
					globalCtx->envCtx.screenFillColor[0] = 160;
					globalCtx->envCtx.screenFillColor[1] = 160;
					globalCtx->envCtx.screenFillColor[2] = 160;
					
					if (globalCtx->sceneLoadFlag != TRANS_TRIGGER_END) {
						globalCtx->envCtx.screenFillColor[3] = 0;
						globalCtx->transitionMode = TRANS_MODE_FILL_IN;
					} else {
						globalCtx->envCtx.screenFillColor[3] = 255;
						globalCtx->transitionMode = TRANS_MODE_FILL_OUT;
					}
					break;
					
				case TRANS_MODE_FILL_IN:
					globalCtx->envCtx.screenFillColor[3] = (D_801614C8 / 20.0f) * 255.0f;
					
					if (D_801614C8 >= 20) {
						globalCtx->state.running = false;
						SET_NEXT_GAMESTATE(&globalCtx->state, Gameplay_Init, GlobalContext);
						gSaveContext.entranceIndex = globalCtx->nextEntranceIndex;
						globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
						globalCtx->transitionMode = TRANS_MODE_OFF;
					} else {
						D_801614C8++;
					}
					break;
					
				case TRANS_MODE_FILL_OUT:
					globalCtx->envCtx.screenFillColor[3] = (1 - D_801614C8 / 20.0f) * 255.0f;
					
					if (D_801614C8 >= 20) {
						gTrnsnUnkState = 0;
						R_UPDATE_RATE = 3;
						globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
						globalCtx->transitionMode = TRANS_MODE_OFF;
						globalCtx->envCtx.fillScreen = false;
					} else {
						D_801614C8++;
					}
					break;
					
				case TRANS_MODE_FILL_BROWN_INIT:
					D_801614C8 = 0;
					globalCtx->envCtx.fillScreen = true;
					globalCtx->envCtx.screenFillColor[0] = 170;
					globalCtx->envCtx.screenFillColor[1] = 160;
					globalCtx->envCtx.screenFillColor[2] = 150;
					
					if (globalCtx->sceneLoadFlag != TRANS_TRIGGER_END) {
						globalCtx->envCtx.screenFillColor[3] = 0;
						globalCtx->transitionMode = TRANS_MODE_FILL_IN;
					} else {
						globalCtx->envCtx.screenFillColor[3] = 255;
						globalCtx->transitionMode = TRANS_MODE_FILL_OUT;
					}
					break;
					
				case TRANS_MODE_INSTANT:
					if (globalCtx->sceneLoadFlag != TRANS_TRIGGER_END) {
						globalCtx->state.running = false;
						SET_NEXT_GAMESTATE(&globalCtx->state, Gameplay_Init, GlobalContext);
						gSaveContext.entranceIndex = globalCtx->nextEntranceIndex;
						globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
						globalCtx->transitionMode = TRANS_MODE_OFF;
					} else {
						gTrnsnUnkState = 0;
						R_UPDATE_RATE = 3;
						globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
						globalCtx->transitionMode = TRANS_MODE_OFF;
					}
					break;
					
				case TRANS_MODE_INSTANCE_WAIT:
					if (gSaveContext.unk_1410 != 0) {
						globalCtx->transitionMode = TRANS_MODE_INSTANCE_RUNNING;
					}
					break;
					
				case TRANS_MODE_SANDSTORM_INIT:
					if (globalCtx->sceneLoadFlag != TRANS_TRIGGER_END) {
						// trigger in, leaving area
						globalCtx->envCtx.sandstormState = SANDSTORM_FILL;
						globalCtx->transitionMode = TRANS_MODE_SANDSTORM;
					} else {
						globalCtx->envCtx.sandstormState = SANDSTORM_UNFILL;
						globalCtx->envCtx.sandstormPrimA = 255;
						globalCtx->envCtx.sandstormEnvA = 255;
						globalCtx->transitionMode = TRANS_MODE_SANDSTORM;
					}
					break;
					
				case TRANS_MODE_SANDSTORM:
					Audio_PlaySoundGeneral(
						NA_SE_EV_SAND_STORM - SFX_FLAG,
						&D_801333D4,
						4,
						&D_801333E0,
						&D_801333E0,
						&D_801333E8
					);
					
					if (globalCtx->sceneLoadFlag == TRANS_TRIGGER_END) {
						if (globalCtx->envCtx.sandstormPrimA < 110) {
							gTrnsnUnkState = 0;
							R_UPDATE_RATE = 3;
							globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
							globalCtx->transitionMode = TRANS_MODE_OFF;
						}
					} else {
						if (globalCtx->envCtx.sandstormEnvA == 255) {
							globalCtx->state.running = false;
							SET_NEXT_GAMESTATE(&globalCtx->state, Gameplay_Init, GlobalContext);
							gSaveContext.entranceIndex = globalCtx->nextEntranceIndex;
							globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
							globalCtx->transitionMode = TRANS_MODE_OFF;
						}
					}
					break;
					
				case TRANS_MODE_SANDSTORM_END_INIT:
					if (globalCtx->sceneLoadFlag == TRANS_TRIGGER_END) {
						globalCtx->envCtx.sandstormState = SANDSTORM_DISSIPATE;
						globalCtx->envCtx.sandstormPrimA = 255;
						globalCtx->envCtx.sandstormEnvA = 255;
						globalCtx->transitionMode = TRANS_MODE_SANDSTORM_END;
					} else {
						globalCtx->transitionMode = TRANS_MODE_SANDSTORM_INIT;
					}
					break;
					
				case TRANS_MODE_SANDSTORM_END:
					Audio_PlaySoundGeneral(
						NA_SE_EV_SAND_STORM - SFX_FLAG,
						&D_801333D4,
						4,
						&D_801333E0,
						&D_801333E0,
						&D_801333E8
					);
					if (globalCtx->sceneLoadFlag == TRANS_TRIGGER_END) {
						if (globalCtx->envCtx.sandstormPrimA <= 0) {
							gTrnsnUnkState = 0;
							R_UPDATE_RATE = 3;
							globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
							globalCtx->transitionMode = TRANS_MODE_OFF;
						}
					}
					break;
					
				case TRANS_MODE_CS_BLACK_FILL_INIT:
					D_801614C8 = 0;
					globalCtx->envCtx.fillScreen = true;
					globalCtx->envCtx.screenFillColor[0] = 0;
					globalCtx->envCtx.screenFillColor[1] = 0;
					globalCtx->envCtx.screenFillColor[2] = 0;
					globalCtx->envCtx.screenFillColor[3] = 255;
					globalCtx->transitionMode = TRANS_MODE_CS_BLACK_FILL;
					break;
					
				case TRANS_MODE_CS_BLACK_FILL:
					if (gSaveContext.unk_1410 != 0) {
						globalCtx->envCtx.screenFillColor[3] = gSaveContext.unk_1410;
						if (gSaveContext.unk_1410 <= 100) {
							gTrnsnUnkState = 0;
							R_UPDATE_RATE = 3;
							globalCtx->sceneLoadFlag = TRANS_TRIGGER_OFF;
							globalCtx->transitionMode = TRANS_MODE_OFF;
						}
					}
					break;
			}
		}
		
		if (1 && (gTrnsnUnkState != 3)) {
			if ((gSaveContext.gameMode == 0) && (globalCtx->msgCtx.msgMode == MSGMODE_NONE) &&
				(globalCtx->gameOverCtx.state == GAMEOVER_INACTIVE)) {
				KaleidoSetup_Update(globalCtx);
			}
			
			sp80 = (globalCtx->pauseCtx.state != 0) || (globalCtx->pauseCtx.debugState != 0);
			AnimationContext_Reset(&globalCtx->animationCtx);
			Object_UpdateBank(&globalCtx->objectCtx);
			
			if ((sp80 == 0) && (IREG(72) == 0)) {
				globalCtx->gameplayFrames++;
				
				func_800AA178(1);
				
				if (globalCtx->actorCtx.freezeFlashTimer && (globalCtx->actorCtx.freezeFlashTimer-- < 5)) {
					if ((globalCtx->actorCtx.freezeFlashTimer > 0) &&
						((globalCtx->actorCtx.freezeFlashTimer % 2) != 0)) {
						globalCtx->envCtx.fillScreen = true;
						globalCtx->envCtx.screenFillColor[0] = globalCtx->envCtx.screenFillColor[1] =
							globalCtx->envCtx.screenFillColor[2] = 150;
						globalCtx->envCtx.screenFillColor[3] = 80;
					} else {
						globalCtx->envCtx.fillScreen = false;
					}
				} else {
					func_800973FC(globalCtx, &globalCtx->roomCtx);
					CollisionCheck_AT(globalCtx, &globalCtx->colChkCtx);
					CollisionCheck_OC(globalCtx, &globalCtx->colChkCtx);
					CollisionCheck_Damage(globalCtx, &globalCtx->colChkCtx);
					CollisionCheck_ClearContext(globalCtx, &globalCtx->colChkCtx);
					
					if (!globalCtx->unk_11DE9) {
						Actor_UpdateAll(globalCtx, &globalCtx->actorCtx);
					}
					
					func_80064558(globalCtx, &globalCtx->csCtx);
					func_800645A0(globalCtx, &globalCtx->csCtx);
					Effect_UpdateAll(globalCtx);
					EffectSs_UpdateAll(globalCtx);
				}
			} else {
				func_800AA178(0);
			}
			
			func_80095AA0(globalCtx, &globalCtx->roomCtx.curRoom, &input[1], 0);
			func_80095AA0(globalCtx, &globalCtx->roomCtx.prevRoom, &input[1], 1);
			
			if (globalCtx->unk_1242B != 0) {
				if (CHECK_BTN_ALL(input[0].press.button, BTN_CUP)) {
					if ((globalCtx->pauseCtx.state != 0) || (globalCtx->pauseCtx.debugState != 0)) {
					} else if (Player_InCsMode(globalCtx)) {
					} else if (YREG(15) == 0x10) {
						Audio_PlaySoundGeneral(
							NA_SE_SY_ERROR,
							&D_801333D4,
							4,
							&D_801333E0,
							&D_801333E0,
							&D_801333E8
						);
					} else {
						func_800BC490(globalCtx, globalCtx->unk_1242B ^ 3);
					}
				}
				func_800BC450(globalCtx);
			}
			
			SkyboxDraw_Update(&globalCtx->skyboxCtx);
			
			if ((globalCtx->pauseCtx.state != 0) || (globalCtx->pauseCtx.debugState != 0)) {
				KaleidoScopeCall_Update(globalCtx);
			} else if (globalCtx->gameOverCtx.state != GAMEOVER_INACTIVE) {
				GameOver_Update(globalCtx);
			} else {
				Message_Update(globalCtx);
			}
			
			Interface_Update(globalCtx);
			AnimationContext_Update(globalCtx, &globalCtx->animationCtx);
			SoundSource_UpdateAll(globalCtx);
			ShrinkWindow_Update(R_UPDATE_RATE);
			TransitionFade_Update(&globalCtx->transitionFade, R_UPDATE_RATE);
		} else {
			goto skip;
		}
	}
skip:
	
	if ((sp80 == 0) || (gDbgCamEnabled)) {
		s32 i;
		
		globalCtx->nextCamera = globalCtx->activeCamera;
		
		for (i = 0; i < NUM_CAMS; i++) {
			if ((i != globalCtx->nextCamera) && (globalCtx->cameraPtrs[i] != NULL)) {
				Camera_Update(globalCtx->cameraPtrs[i]);
			}
		}
		
		Camera_Update(globalCtx->cameraPtrs[globalCtx->nextCamera]);
	}
	
	Environment_Update(
		globalCtx,
		&globalCtx->envCtx,
		&globalCtx->lightCtx,
		&globalCtx->pauseCtx,
		&globalCtx->msgCtx,
		&globalCtx->gameOverCtx,
		globalCtx->state.gfxCtx
	);
}

void Play_Main(GlobalContext* globalCtx) {
	D_8012D1F8 = &globalCtx->state.input[0];
	
	if ((HREG(80) == 10) && (HREG(94) != 10)) {
		HREG(81) = 1;
		HREG(82) = 1;
		HREG(83) = 1;
		HREG(84) = 3;
		HREG(85) = 1;
		HREG(86) = 1;
		HREG(87) = 1;
		HREG(88) = 1;
		HREG(89) = 1;
		HREG(90) = 15;
		HREG(91) = 1;
		HREG(92) = 1;
		HREG(93) = 1;
		HREG(94) = 10;
	}
	
	if ((HREG(80) != 10) || (HREG(81) != 0))
		Gameplay_Update(globalCtx);
	Gameplay_Draw(globalCtx);
	
#ifdef DEV_BUILD
	DebugMenu_Update(globalCtx);
#endif
}