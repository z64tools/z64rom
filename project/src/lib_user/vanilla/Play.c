#include <uLib.h>
#include "code/z_play.h"
#include <code/z_scene_table.h>

asm ("Opening_Init = 0x80803CAC");
asm ("FileChoose_Init = 0x80811A20");

void NewPlay_SpawnScene(PlayState* playState, s32 sceneNum, s32 spawn) {
	SceneTableEntry* scene = &gSceneTable[sceneNum];
	u32 roomSize;
	
	scene->unk_13 = 0;
	playState->loadedScene = scene;
	playState->sceneNum = sceneNum;
	playState->sceneDrawConfig = scene->drawConfig;
	
	playState->sceneSegment = Play_LoadFile(playState, &scene->sceneFile);
	scene->unk_13 = 0;
	
	gSegments[2] = VIRTUAL_TO_PHYSICAL(playState->sceneSegment);
	
	Play_InitScene(playState, spawn);
	roomSize = func_80096FE8(playState, &playState->roomCtx);
	sSceneDrawConfigs[4] = SceneAnim_Update;
	
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
		playState->sceneSegment
	);
	osLibPrintf(
		"Room Size "
		PRNT_YELW "%.1f " PRNT_RSET
		"KB",
		BinToKb(roomSize)
	);
}

void NewPlay_Init(PlayState* playState) {
	GraphicsContext* gfxCtx = playState->state.gfxCtx;
	u32 zAlloc;
	u32 zAllocAligned;
	size_t zAllocSize;
	Player* player;
	s32 playerStartCamId;
	s32 i;
	u8 tempSetupIndex;
	
	if (gSaveContext.entranceIndex == -1) {
		gSaveContext.entranceIndex = 0;
		playState->state.running = false;
		SET_NEXT_GAMESTATE(&playState->state, Opening_Init, OpeningContext);
		
		return;
	}
	
	osLibPrintf("Entrance Index: %04X", gExitParam.nextEntranceIndex);
	osLibPrintf(
		"gExitParam:\n"
		PRNT_RSET "\tflag:       %s\n"
		PRNT_RSET "\tmusicOn:    %s\n"
		PRNT_RSET "\ttitleCard:  %s\n"
		PRNT_RSET "\tfadeIn:     " PRNT_BLUE " 0x%02X\n"
		PRNT_RSET "\tfadeOut:    " PRNT_BLUE " 0x%02X\n"
		PRNT_RSET "\tspawnIndex: " PRNT_BLUE " 0x%02X\n"
		PRNT_RSET "\theaderIndex:" PRNT_BLUE " 0x%02X\n"
		PRNT_RSET "\tsceneIndex: " PRNT_BLUE " 0x%02X",
		gExitParam.isExit ? PRNT_GREN " true" : PRNT_REDD "false",
		gExitParam.exit.musicOn ? PRNT_GREN " true" : PRNT_REDD "false",
		gExitParam.exit.titleCard ? PRNT_GREN " true" : PRNT_REDD "false",
		gExitParam.exit.fadeIn,
		gExitParam.exit.fadeOut,
		gExitParam.exit.spawnIndex,
		gExitParam.exit.headerIndex,
		gExitParam.exit.sceneIndex
	);
	
	SystemArena_Display();
	GameState_Realloc(&playState->state, 0x3D8000 + 0x3A60);
	KaleidoManager_Init(playState);
	View_Init(&playState->view, gfxCtx);
	Audio_SetExtraFilter(0);
	Quake_Init();
	
	for (i = 0; i < ARRAY_COUNT(playState->cameraPtrs); i++) {
		playState->cameraPtrs[i] = NULL;
	}
	
	Camera_Init(&playState->mainCamera, &playState->view, &playState->colCtx, playState);
	Camera_ChangeStatus(&playState->mainCamera, CAM_STAT_ACTIVE);
	
	for (i = 0; i < 3; i++) {
		Camera_Init(&playState->subCameras[i], &playState->view, &playState->colCtx, playState);
		Camera_ChangeStatus(&playState->subCameras[i], CAM_STAT_UNK100);
	}
	
	playState->cameraPtrs[CAM_ID_MAIN] = &playState->mainCamera;
	playState->cameraPtrs[CAM_ID_MAIN]->uid = 0;
	playState->activeCamId = CAM_ID_MAIN;
	func_8005AC48(&playState->mainCamera, 0xFF);
	Sram_Init(playState, &playState->sramCtx);
	func_80112098(playState);
	Message_Init(playState);
	GameOver_Init(playState);
	SoundSource_InitAll(playState);
	Effect_InitContext(playState);
	EffectSs_InitInfo(playState, 0x55);
	CollisionCheck_InitContext(playState, &playState->colChkCtx);
	AnimationContext_Reset(&playState->animationCtx);
	func_8006450C(playState, &playState->csCtx);
	
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
	
	Cutscene_HandleConditionalTriggers(playState);
	
	if (gSaveContext.gameMode != 0 || gSaveContext.cutsceneIndex >= 0xFFF0) {
		gSaveContext.nayrusLoveTimer = 0;
		Magic_Reset(playState);
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
	
	u32 sceneNum = 0;
	u32 sceneSpawn = 0;
	
	if (gExitParam.isExit == false) {
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
		
		sceneNum = gEntranceTable[((void)0, gSaveContext.entranceIndex) + ((void)0, gSaveContext.sceneSetupIndex)].scene;
		sceneSpawn = gEntranceTable[((void)0, gSaveContext.sceneSetupIndex) + ((void)0, gSaveContext.entranceIndex)].spawn;
	} else {
		sceneNum = gExitParam.exit.sceneIndex;
		if (gExitParam.exit.headerIndex != 0xF)
			gSaveContext.sceneSetupIndex = gExitParam.exit.headerIndex;
		sceneSpawn = gExitParam.exit.spawnIndex;
	}
	
	NewPlay_SpawnScene(
		playState,
		sceneNum,
		sceneSpawn
	);
	
	Cutscene_HandleEntranceTriggers(playState);
	KaleidoScopeCall_Init(playState);
	func_801109B0(playState);
	
	if (gSaveContext.nextDayTime != 0xFFFF) {
		if (gSaveContext.nextDayTime == 0x8001) {
			gSaveContext.totalDays++;
			gSaveContext.bgsDayCount++;
			gSaveContext.dogIsLost = true;
			if (Inventory_ReplaceItem(playState, ITEM_WEIRD_EGG, ITEM_CHICKEN) ||
				Inventory_ReplaceItem(playState, ITEM_POCKET_EGG, ITEM_POCKET_CUCCO)) {
				Message_StartTextbox(playState, 0x3066, NULL);
			}
			gSaveContext.nextDayTime = 0xFFFE;
		} else {
			gSaveContext.nextDayTime = 0xFFFD;
		}
	}
	
	SREG(91) = -1;
	R_PAUSE_MENU_MODE = 0;
	PreRender_Init(&playState->pauseBgPreRender);
	PreRender_SetValuesSave(&playState->pauseBgPreRender, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0);
	PreRender_SetValues(&playState->pauseBgPreRender, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	gTrnsnUnkState = 0;
	playState->transitionMode = 0;
	FrameAdvance_Init(&playState->frameAdvCtx);
	Rand_Seed((u32)osGetTime());
	Matrix_Init(&playState->state);
	playState->state.main = Play_Main;
	playState->state.destroy = Play_Destroy;
	playState->transitionTrigger = -0x14;
	playState->unk_11E16 = 0xFF;
	playState->unk_11E18 = 0;
	playState->unk_11DE9 = 0;
	
	if (gSaveContext.gameMode != 1) {
		if (gSaveContext.nextTransitionType == TRANS_NEXT_TYPE_DEFAULT) {
			if (gExitParam.isExit == false)
				playState->transitionType = (gEntranceTable[((void)0, gSaveContext.entranceIndex) + tempSetupIndex].field >> 7) & 0x7F;
			else
				playState->transitionType = gExitParam.exit.fadeIn;
		} else {
			playState->transitionType = gSaveContext.nextTransitionType;
			gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
		}
	} else {
		playState->transitionType = TRANS_TYPE_FADE_BLACK_SLOW;
	}
	
	ShrinkWindow_Init();
	TransitionFade_Init(&playState->transitionFade);
	TransitionFade_SetType(&playState->transitionFade, 3);
	TransitionFade_SetColor(&playState->transitionFade, RGBA8(160, 160, 160, 255));
	TransitionFade_Start(&playState->transitionFade);
	VisMono_Init(&D_80161498);
	D_801614B0.a = 0;
	Flags_UnsetAllEnv(playState);
	
	zAllocSize = THA_GetSize(&playState->state.tha);
	zAlloc = (u32)GameState_Alloc(&playState->state, zAllocSize, "../z_play.c", 2918);
	zAllocAligned = (zAlloc + 8) & ~0xF;
	ZeldaArena_Init((void*)zAllocAligned, zAllocSize - zAllocAligned + zAlloc);
	
	Fault_AddClient(&D_801614B8, ZeldaArena_Display, NULL, NULL);
	func_800304DC(playState, &playState->actorCtx, playState->linkActorEntry);
	
	while (!func_800973FC(playState, &playState->roomCtx));
	
	player = GET_PLAYER(playState);
	Camera_InitPlayerSettings(&playState->mainCamera, player);
	Camera_ChangeMode(&playState->mainCamera, CAM_MODE_NORMAL);
	
	playerStartCamId = player->actor.params & 0xFF;
	if (playerStartCamId != 0xFF) {
		Camera_ChangeDataIdx(&playState->mainCamera, playerStartCamId);
	}
	
	if (YREG(15) == 32) {
		playState->unk_1242B = 2;
	} else if (YREG(15) == 16) {
		playState->unk_1242B = 1;
	} else {
		playState->unk_1242B = 0;
	}
	
	Interface_SetSceneRestrictions(playState);
	Environment_PlaySceneSequence(playState);
	gSaveContext.seqId = playState->sequenceCtx.seqId;
	gSaveContext.natureAmbienceId = playState->sequenceCtx.natureAmbienceId;
	func_8002DF18(playState, GET_PLAYER(playState));
	AnimationContext_Update(playState, &playState->animationCtx);
	gSaveContext.respawnFlag = 0;
	
	if (dREG(95) != 0) {
		D_8012D1F0 = D_801614D0;
		osSyncPrintf("\nkawauso_data=[%x]", D_8012D1F0);
		DmaMgr_DmaRomToRam(0x03FEB000, (u32)D_8012D1F0, sizeof(D_801614D0));
	}
}

void NewPlay_Draw(PlayState* playState) {
	GraphicsContext* gfxCtx = playState->state.gfxCtx;
	Vec3f sp21C;
	
	POLY_OPA_DISP = Play_SetFog(playState, POLY_OPA_DISP);
	POLY_XLU_DISP = Play_SetFog(playState, POLY_XLU_DISP);
	
	View_SetPerspective(&playState->view, playState->view.fovy, playState->view.zNear, playState->lightCtx.fogFar);
	View_Apply(&playState->view, VIEW_ALL);
	
	// The billboard matrix temporarily stores the viewing matrix
	Matrix_MtxToMtxF(&playState->view.viewing, &playState->billboardMtxF);
	Matrix_MtxToMtxF(&playState->view.projection, &playState->viewProjectionMtxF);
	Matrix_Mult(&playState->viewProjectionMtxF, MTXMODE_NEW);
	// The billboard is still a viewing matrix at this stage
	Matrix_Mult(&playState->billboardMtxF, MTXMODE_APPLY);
	Matrix_Get(&playState->viewProjectionMtxF);
	playState->billboardMtxF.mf[0][3] = playState->billboardMtxF.mf[1][3] = playState->billboardMtxF.mf[2][3] =
		playState->billboardMtxF.mf[3][0] = playState->billboardMtxF.mf[3][1] = playState->billboardMtxF.mf[3][2] =
		0.0f;
	// This transpose is where the viewing matrix is properly converted into a billboard matrix
	Matrix_Transpose(&playState->billboardMtxF);
	playState->billboardMtx = Matrix_MtxFToMtx(
		Matrix_CheckFloats(&playState->billboardMtxF, "../z_play.c", 4005),
		Graph_Alloc(gfxCtx, sizeof(Mtx))
	);
	
	gSPSegment(POLY_OPA_DISP++, 0x01, playState->billboardMtx);
	
	if ((HREG(80) != 10) || (HREG(92) != 0)) {
		Gfx* gfxP;
		Gfx* sp1CC = POLY_OPA_DISP;
		
		gfxP = Graph_GfxPlusOne(sp1CC);
		gSPDisplayList(OVERLAY_DISP++, gfxP);
		
		if ((playState->transitionMode == TRANS_MODE_INSTANCE_RUNNING) ||
			(playState->transitionMode == TRANS_MODE_INSTANCE_WAIT) ||
			(playState->transitionCtx.transitionType >= 56)) {
			View view;
			
			View_Init(&view, gfxCtx);
			view.flags = VIEW_VIEWPORT | VIEW_PROJECTION_ORTHO;
			
			SET_FULLSCREEN_VIEWPORT(&view);
			
			View_ApplyTo(&view, VIEW_ALL, &gfxP);
			playState->transitionCtx.draw(&playState->transitionCtx.instanceData, &gfxP);
		}
		
		TransitionFade_Draw(&playState->transitionFade, &gfxP);
		
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
			&playState->pauseBgPreRender,
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			gfxCtx->curFrameBuffer,
			gZBuffer
		);
		
		if (R_PAUSE_MENU_MODE == 2) {
			Sched_FlushTaskQueue();
			PreRender_Calc(&playState->pauseBgPreRender);
			R_PAUSE_MENU_MODE = 3;
		} else if (R_PAUSE_MENU_MODE >= 4) {
			R_PAUSE_MENU_MODE = 0;
		}
		
		if (R_PAUSE_MENU_MODE == 3) {
			Gfx* sp84 = POLY_OPA_DISP;
			
			func_800C24BC(&playState->pauseBgPreRender, &sp84);
			POLY_OPA_DISP = sp84;
			goto Gameplay_Draw_DrawOverlayElements;
		} else {
			s32 sp80 = 0;
			
			if ((HREG(80) != 10) || (HREG(83) != 0)) {
				if (playState->skyboxId && (playState->skyboxId != SKYBOX_UNSET_1D) &&
					!playState->envCtx.skyboxDisabled) {
					if ((playState->skyboxId == SKYBOX_NORMAL_SKY) ||
						(playState->skyboxId == SKYBOX_CUTSCENE_MAP)) {
						Environment_UpdateSkybox(playState->skyboxId, &playState->envCtx, &playState->skyboxCtx);
						SkyboxDraw_Draw(
							&playState->skyboxCtx,
							gfxCtx,
							playState->skyboxId,
							playState->envCtx.skyboxBlend,
							playState->view.eye.x,
							playState->view.eye.y,
							playState->view.eye.z
						);
					} else if (playState->skyboxCtx.unk_140 == 0) {
						SkyboxDraw_Draw(
							&playState->skyboxCtx,
							gfxCtx,
							playState->skyboxId,
							0,
							playState->view.eye.x,
							playState->view.eye.y,
							playState->view.eye.z
						);
					}
				}
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 2)) {
				if (!playState->envCtx.sunMoonDisabled) {
					Environment_DrawSunAndMoon(playState);
				}
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 1)) {
				Environment_DrawSkyboxFilters(playState);
			}
			
			if ((HREG(80) != 10) || (HREG(90) & 4)) {
				Environment_UpdateLightningStrike(playState);
				Environment_DrawLightning(playState, 0);
			}
			
			if ((HREG(80) != 10) || (HREG(84) != 0)) {
				if (VREG(94) == 0) {
					if (HREG(80) != 10) {
						sp80 = 3;
					} else {
						sp80 = HREG(84);
					}
					Scene_Draw(playState);
					NewRoom_Draw(playState, &playState->roomCtx.curRoom, sp80 & 3);
					NewRoom_Draw(playState, &playState->roomCtx.prevRoom, sp80 & 3);
				}
			}
			
			if ((HREG(80) != 10) || (HREG(83) != 0)) {
				if ((playState->skyboxCtx.unk_140 != 0) &&
					(GET_ACTIVE_CAM(playState)->setting != CAM_SET_PREREND_FIXED)) {
					Vec3f sp74;
					
					Camera_GetSkyboxOffset(&sp74, GET_ACTIVE_CAM(playState));
					SkyboxDraw_Draw(
						&playState->skyboxCtx,
						gfxCtx,
						playState->skyboxId,
						0,
						playState->view.eye.x + sp74.x,
						playState->view.eye.y + sp74.y,
						playState->view.eye.z + sp74.z
					);
				}
			}
			
			if (playState->envCtx.precipitation[PRECIP_RAIN_CUR] != 0) {
				Environment_DrawRain(playState, &playState->view, gfxCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(84) != 0)) {
				Environment_FillScreen(gfxCtx, 0, 0, 0, playState->unk_11E18, FILL_SCREEN_OPA);
			}
			
			if ((HREG(80) != 10) || (HREG(85) != 0)) {
				func_800315AC(playState, &playState->actorCtx);
			}
			
			if ((HREG(80) != 10) || (HREG(86) != 0)) {
				if (!playState->envCtx.sunMoonDisabled) {
					sp21C.x = playState->view.eye.x + playState->envCtx.sunPos.x;
					sp21C.y = playState->view.eye.y + playState->envCtx.sunPos.y;
					sp21C.z = playState->view.eye.z + playState->envCtx.sunPos.z;
					Environment_DrawSunLensFlare(playState, &playState->envCtx, &playState->view, gfxCtx, sp21C, 0);
				}
				Environment_DrawCustomLensFlare(playState);
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
				
				switch (playState->envCtx.fillScreen) {
					case 1:
						Environment_FillScreen(
							gfxCtx,
							playState->envCtx.screenFillColor[0],
							playState->envCtx.screenFillColor[1],
							playState->envCtx.screenFillColor[2],
							playState->envCtx.screenFillColor[3],
							FILL_SCREEN_OPA | FILL_SCREEN_XLU
						);
						break;
					default:
						break;
				}
			}
			
			if ((HREG(80) != 10) || (HREG(88) != 0)) {
				if (playState->envCtx.sandstormState != 0) {
					Environment_DrawSandstorm(playState, playState->envCtx.sandstormState);
				}
			}
			
			if ((R_PAUSE_MENU_MODE == 1) || (gTrnsnUnkState == 1)) {
				Gfx* sp70 = OVERLAY_DISP;
				
				playState->pauseBgPreRender.fbuf = gfxCtx->curFrameBuffer;
				playState->pauseBgPreRender.fbufSave = (u16*)gZBuffer;
				func_800C1F20(&playState->pauseBgPreRender, &sp70);
				if (R_PAUSE_MENU_MODE == 1) {
					playState->pauseBgPreRender.cvgSave = (u8*)gfxCtx->curFrameBuffer;
					func_800C20B4(&playState->pauseBgPreRender, &sp70);
					R_PAUSE_MENU_MODE = 3;
				} else {
					gTrnsnUnkState = 2;
				}
				OVERLAY_DISP = sp70;
				playState->unk_121C7 = 2;
				SREG(33) |= 1;
			} else {
Gameplay_Draw_DrawOverlayElements:
				if ((HREG(80) != 10) || (HREG(89) != 0)) {
					Play_DrawOverlayElements(playState);
				}
			}
		}
	}
}

void NewPlay_Update(PlayState* playState) {
	s32 sp80 = 0;
	Input* input;
	
	// u16 neEnIn = playState->nextEntranceIndex;
	
	input = playState->state.input;
	
	gSegments[4] = VIRTUAL_TO_PHYSICAL(playState->objectCtx.status[playState->objectCtx.mainKeepIndex].segment);
	gSegments[5] = VIRTUAL_TO_PHYSICAL(playState->objectCtx.status[playState->objectCtx.subKeepIndex].segment);
	gSegments[2] = VIRTUAL_TO_PHYSICAL(playState->sceneSegment);
	
	if (FrameAdvance_Update(&playState->frameAdvCtx, &input[1])) {
		if ((playState->transitionMode == TRANS_MODE_OFF) && (playState->transitionTrigger != TRANS_TRIGGER_OFF)) {
			playState->transitionMode = TRANS_MODE_SETUP;
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
		
		if (playState->transitionMode) {
			switch (playState->transitionMode) {
				case TRANS_MODE_SETUP:
					if (playState->transitionTrigger != TRANS_TRIGGER_END) {
						s16 sceneSetupIndex = 0;
						s16 fadeOut = false;
						
						Interface_ChangeAlpha(1);
						
						if (gSaveContext.cutsceneIndex >= 0xFFF0) {
							sceneSetupIndex = (gSaveContext.cutsceneIndex & 0xF) + 4;
						}
						
						// z64rom
						
						if (gExitParam.isExit == false) {
							if (!(gEntranceTable[gExitParam.nextEntranceIndex + sceneSetupIndex].field & 0x8000))
								fadeOut = true;
						} else {
							if (gExitParam.exit.musicOn == false)
								fadeOut = true;
						}
						
						// fade out bgm if "continue bgm" flag is not set
						if (fadeOut) {
							if ((playState->transitionType < TRANS_TYPE_MAX) &&
								!Environment_IsForcedSequenceDisabled()) {
								func_800F6964(0x14);
								gSaveContext.seqId = (u8)NA_BGM_DISABLED;
								gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
							}
						}
					}
					
					if (!R_TRANS_DBG_ENABLED)
						Play_SetupTransition(playState, playState->transitionType);
					else
						Play_SetupTransition(playState, R_TRANS_DBG_TYPE);
					
					if (playState->transitionMode >= TRANS_MODE_FILL_WHITE_INIT) {
						// non-instance modes break out of this switch
						break;
					}
				// fallthrough
				case TRANS_MODE_INSTANCE_INIT:
					playState->transitionCtx.init(&playState->transitionCtx.instanceData);
					
					// circle types
					if ((playState->transitionCtx.transitionType >> 5) == 1) {
						playState->transitionCtx.setType(
							&playState->transitionCtx.instanceData,
							playState->transitionCtx.transitionType | TC_SET_PARAMS
						);
					}
					
					gSaveContext.transWipeSpeed = 14;
					
					if ((playState->transitionCtx.transitionType == TRANS_TYPE_WIPE_FAST) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FILL_WHITE2)) {
						//! @bug TRANS_TYPE_FILL_WHITE2 will never reach this code.
						//! It is a non-instance type transition which doesn't run this case.
						gSaveContext.transWipeSpeed = 28;
					}
					
					gSaveContext.transFadeDuration = 60;
					
					if ((playState->transitionCtx.transitionType == TRANS_TYPE_FADE_BLACK_FAST) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_FAST)) {
						gSaveContext.transFadeDuration = 20;
					} else if ((playState->transitionCtx.transitionType == TRANS_TYPE_FADE_BLACK_SLOW) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_SLOW)) {
						gSaveContext.transFadeDuration = 150;
					} else if (playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_INSTANT) {
						gSaveContext.transFadeDuration = 2;
					}
					
					if ((playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_FAST) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_SLOW) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_CS_DELAYED) ||
						(playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_INSTANT)) {
						playState->transitionCtx.setColor(
							&playState->transitionCtx.instanceData,
							RGBA8(160, 160, 160, 255)
						);
						if (playState->transitionCtx.setUnkColor != NULL) {
							playState->transitionCtx.setUnkColor(
								&playState->transitionCtx.instanceData,
								RGBA8(160, 160, 160, 255)
							);
						}
					} else if (playState->transitionCtx.transitionType == TRANS_TYPE_FADE_GREEN) {
						playState->transitionCtx.setColor(
							&playState->transitionCtx.instanceData,
							RGBA8(140, 140, 100, 255)
						);
						
						if (playState->transitionCtx.setUnkColor != NULL) {
							playState->transitionCtx.setUnkColor(
								&playState->transitionCtx.instanceData,
								RGBA8(140, 140, 100, 255)
							);
						}
					} else if (playState->transitionCtx.transitionType == TRANS_TYPE_FADE_BLUE) {
						playState->transitionCtx.setColor(
							&playState->transitionCtx.instanceData,
							RGBA8(70, 100, 110, 255)
						);
						
						if (playState->transitionCtx.setUnkColor != NULL) {
							playState->transitionCtx.setUnkColor(
								&playState->transitionCtx.instanceData,
								RGBA8(70, 100, 110, 255)
							);
						}
					} else {
						playState->transitionCtx.setColor(&playState->transitionCtx.instanceData, RGBA8(0, 0, 0, 0));
						
						if (playState->transitionCtx.setUnkColor != NULL) {
							playState->transitionCtx.setUnkColor(
								&playState->transitionCtx.instanceData,
								RGBA8(0, 0, 0, 0)
							);
						}
					}
					
					if (playState->transitionTrigger == TRANS_TRIGGER_END) {
						playState->transitionCtx.setType(&playState->transitionCtx.instanceData, 1);
					} else {
						playState->transitionCtx.setType(&playState->transitionCtx.instanceData, 2);
					}
					
					playState->transitionCtx.start(&playState->transitionCtx.instanceData);
					
					if (playState->transitionCtx.transitionType == TRANS_TYPE_FADE_WHITE_CS_DELAYED) {
						playState->transitionMode = TRANS_MODE_INSTANCE_WAIT;
					} else {
						playState->transitionMode = TRANS_MODE_INSTANCE_RUNNING;
					}
					break;
					
				case TRANS_MODE_INSTANCE_RUNNING:
					if (playState->transitionCtx.isDone(&playState->transitionCtx.instanceData)) {
						if (playState->transitionCtx.transitionType >= TRANS_TYPE_MAX) {
							if (playState->transitionTrigger == TRANS_TRIGGER_END) {
								playState->transitionCtx.destroy(&playState->transitionCtx.instanceData);
								func_800BC88C(playState);
								playState->transitionMode = TRANS_MODE_OFF;
							}
						} else if (playState->transitionTrigger != TRANS_TRIGGER_END) {
							playState->state.running = false;
							
							if (gSaveContext.gameMode != 2) {
								SET_NEXT_GAMESTATE(&playState->state, Play_Init, PlayState);
								gSaveContext.entranceIndex = gExitParam.nextEntranceIndex;
								
								if (gSaveContext.minigameState == 1) {
									gSaveContext.minigameState = 3;
								}
							} else {
								SET_NEXT_GAMESTATE(&playState->state, FileChoose_Init, FileChooseContext);
							}
						} else {
							playState->transitionCtx.destroy(&playState->transitionCtx.instanceData);
							func_800BC88C(playState);
							playState->transitionMode = TRANS_MODE_OFF;
							
							if (gTrnsnUnkState == 3) {
								TransitionUnk_Destroy(&sTrnsnUnk);
								gTrnsnUnkState = 0;
								R_UPDATE_RATE = 3;
							}
						}
						
						playState->transitionTrigger = TRANS_TRIGGER_OFF;
					} else {
						playState->transitionCtx.update(&playState->transitionCtx.instanceData, R_UPDATE_RATE);
					}
					break;
			}
			
			// update non-instance transitions
			switch (playState->transitionMode) {
				case TRANS_MODE_FILL_WHITE_INIT:
					sTransitionFillTimer = 0;
					playState->envCtx.fillScreen = true;
					playState->envCtx.screenFillColor[0] = 160;
					playState->envCtx.screenFillColor[1] = 160;
					playState->envCtx.screenFillColor[2] = 160;
					
					if (playState->transitionTrigger != TRANS_TRIGGER_END) {
						playState->envCtx.screenFillColor[3] = 0;
						playState->transitionMode = TRANS_MODE_FILL_IN;
					} else {
						playState->envCtx.screenFillColor[3] = 255;
						playState->transitionMode = TRANS_MODE_FILL_OUT;
					}
					break;
					
				case TRANS_MODE_FILL_IN:
					playState->envCtx.screenFillColor[3] = (sTransitionFillTimer / 20.0f) * 255.0f;
					
					if (sTransitionFillTimer >= 20) {
						playState->state.running = false;
						SET_NEXT_GAMESTATE(&playState->state, Play_Init, PlayState);
						gSaveContext.entranceIndex = gExitParam.nextEntranceIndex;
						playState->transitionTrigger = TRANS_TRIGGER_OFF;
						playState->transitionMode = TRANS_MODE_OFF;
					} else {
						sTransitionFillTimer++;
					}
					break;
					
				case TRANS_MODE_FILL_OUT:
					playState->envCtx.screenFillColor[3] = (1 - sTransitionFillTimer / 20.0f) * 255.0f;
					
					if (sTransitionFillTimer >= 20) {
						gTrnsnUnkState = 0;
						R_UPDATE_RATE = 3;
						playState->transitionTrigger = TRANS_TRIGGER_OFF;
						playState->transitionMode = TRANS_MODE_OFF;
						playState->envCtx.fillScreen = false;
					} else {
						sTransitionFillTimer++;
					}
					break;
					
				case TRANS_MODE_FILL_BROWN_INIT:
					sTransitionFillTimer = 0;
					playState->envCtx.fillScreen = true;
					playState->envCtx.screenFillColor[0] = 170;
					playState->envCtx.screenFillColor[1] = 160;
					playState->envCtx.screenFillColor[2] = 150;
					
					if (playState->transitionTrigger != TRANS_TRIGGER_END) {
						playState->envCtx.screenFillColor[3] = 0;
						playState->transitionMode = TRANS_MODE_FILL_IN;
					} else {
						playState->envCtx.screenFillColor[3] = 255;
						playState->transitionMode = TRANS_MODE_FILL_OUT;
					}
					break;
					
				case TRANS_MODE_INSTANT:
					if (playState->transitionTrigger != TRANS_TRIGGER_END) {
						playState->state.running = false;
						SET_NEXT_GAMESTATE(&playState->state, Play_Init, PlayState);
						gSaveContext.entranceIndex = gExitParam.nextEntranceIndex;
						playState->transitionTrigger = TRANS_TRIGGER_OFF;
						playState->transitionMode = TRANS_MODE_OFF;
					} else {
						gTrnsnUnkState = 0;
						R_UPDATE_RATE = 3;
						playState->transitionTrigger = TRANS_TRIGGER_OFF;
						playState->transitionMode = TRANS_MODE_OFF;
					}
					break;
					
				case TRANS_MODE_INSTANCE_WAIT:
					if (gSaveContext.cutsceneTransitionControl != 0) {
						playState->transitionMode = TRANS_MODE_INSTANCE_RUNNING;
					}
					break;
					
				case TRANS_MODE_SANDSTORM_INIT:
					if (playState->transitionTrigger != TRANS_TRIGGER_END) {
						// trigger in, leaving area
						playState->envCtx.sandstormState = SANDSTORM_FILL;
						playState->transitionMode = TRANS_MODE_SANDSTORM;
					} else {
						playState->envCtx.sandstormState = SANDSTORM_UNFILL;
						playState->envCtx.sandstormPrimA = 255;
						playState->envCtx.sandstormEnvA = 255;
						playState->transitionMode = TRANS_MODE_SANDSTORM;
					}
					break;
					
				case TRANS_MODE_SANDSTORM:
					Audio_PlaySoundGeneral(
						NA_SE_EV_SAND_STORM - SFX_FLAG,
						&gSfxDefaultPos,
						4,
						&gSfxDefaultFreqAndVolScale,
						&gSfxDefaultFreqAndVolScale,
						&gSfxDefaultReverb
					);
					
					if (playState->transitionTrigger == TRANS_TRIGGER_END) {
						if (playState->envCtx.sandstormPrimA < 110) {
							gTrnsnUnkState = 0;
							R_UPDATE_RATE = 3;
							playState->transitionTrigger = TRANS_TRIGGER_OFF;
							playState->transitionMode = TRANS_MODE_OFF;
						}
					} else {
						if (playState->envCtx.sandstormEnvA == 255) {
							playState->state.running = false;
							SET_NEXT_GAMESTATE(&playState->state, Play_Init, PlayState);
							gSaveContext.entranceIndex = gExitParam.nextEntranceIndex;
							playState->transitionTrigger = TRANS_TRIGGER_OFF;
							playState->transitionMode = TRANS_MODE_OFF;
						}
					}
					break;
					
				case TRANS_MODE_SANDSTORM_END_INIT:
					if (playState->transitionTrigger == TRANS_TRIGGER_END) {
						playState->envCtx.sandstormState = SANDSTORM_DISSIPATE;
						playState->envCtx.sandstormPrimA = 255;
						playState->envCtx.sandstormEnvA = 255;
						playState->transitionMode = TRANS_MODE_SANDSTORM_END;
					} else {
						playState->transitionMode = TRANS_MODE_SANDSTORM_INIT;
					}
					break;
					
				case TRANS_MODE_SANDSTORM_END:
					Audio_PlaySoundGeneral(
						NA_SE_EV_SAND_STORM - SFX_FLAG,
						&gSfxDefaultPos,
						4,
						&gSfxDefaultFreqAndVolScale,
						&gSfxDefaultFreqAndVolScale,
						&gSfxDefaultReverb
					);
					if (playState->transitionTrigger == TRANS_TRIGGER_END) {
						if (playState->envCtx.sandstormPrimA <= 0) {
							gTrnsnUnkState = 0;
							R_UPDATE_RATE = 3;
							playState->transitionTrigger = TRANS_TRIGGER_OFF;
							playState->transitionMode = TRANS_MODE_OFF;
						}
					}
					break;
					
				case TRANS_MODE_CS_BLACK_FILL_INIT:
					sTransitionFillTimer = 0;
					playState->envCtx.fillScreen = true;
					playState->envCtx.screenFillColor[0] = 0;
					playState->envCtx.screenFillColor[1] = 0;
					playState->envCtx.screenFillColor[2] = 0;
					playState->envCtx.screenFillColor[3] = 255;
					playState->transitionMode = TRANS_MODE_CS_BLACK_FILL;
					break;
					
				case TRANS_MODE_CS_BLACK_FILL:
					if (gSaveContext.cutsceneTransitionControl != 0) {
						playState->envCtx.screenFillColor[3] = gSaveContext.cutsceneTransitionControl;
						if (gSaveContext.cutsceneTransitionControl <= 100) {
							gTrnsnUnkState = 0;
							R_UPDATE_RATE = 3;
							playState->transitionTrigger = TRANS_TRIGGER_OFF;
							playState->transitionMode = TRANS_MODE_OFF;
						}
					}
					break;
			}
		}
		
		if (1 && (gTrnsnUnkState != 3)) {
			if ((gSaveContext.gameMode == 0) && (playState->msgCtx.msgMode == MSGMODE_NONE) &&
				(playState->gameOverCtx.state == GAMEOVER_INACTIVE)) {
				KaleidoSetup_Update(playState);
			}
			
			sp80 = (playState->pauseCtx.state != 0) || (playState->pauseCtx.debugState != 0);
			AnimationContext_Reset(&playState->animationCtx);
			Object_UpdateBank(&playState->objectCtx);
			
			if ((sp80 == 0) && (IREG(72) == 0)) {
				playState->gameplayFrames++;
				
				func_800AA178(1);
				
				if (playState->actorCtx.freezeFlashTimer && (playState->actorCtx.freezeFlashTimer-- < 5)) {
					if ((playState->actorCtx.freezeFlashTimer > 0) &&
						((playState->actorCtx.freezeFlashTimer % 2) != 0)) {
						playState->envCtx.fillScreen = true;
						playState->envCtx.screenFillColor[0] = playState->envCtx.screenFillColor[1] =
							playState->envCtx.screenFillColor[2] = 150;
						playState->envCtx.screenFillColor[3] = 80;
					} else {
						playState->envCtx.fillScreen = false;
					}
				} else {
					func_800973FC(playState, &playState->roomCtx);
					CollisionCheck_AT(playState, &playState->colChkCtx);
					CollisionCheck_OC(playState, &playState->colChkCtx);
					CollisionCheck_Damage(playState, &playState->colChkCtx);
					CollisionCheck_ClearContext(playState, &playState->colChkCtx);
					
					if (!playState->unk_11DE9) {
						Actor_UpdateAll(playState, &playState->actorCtx);
					}
					
					func_80064558(playState, &playState->csCtx);
					func_800645A0(playState, &playState->csCtx);
					Effect_UpdateAll(playState);
					EffectSs_UpdateAll(playState);
				}
			} else {
				func_800AA178(0);
			}
			
			func_80095AA0(playState, &playState->roomCtx.curRoom, &input[1], 0);
			func_80095AA0(playState, &playState->roomCtx.prevRoom, &input[1], 1);
			
			if (playState->unk_1242B != 0) {
				if (CHECK_BTN_ALL(input[0].press.button, BTN_CUP)) {
					if ((playState->pauseCtx.state != 0) || (playState->pauseCtx.debugState != 0)) {
					} else if (Player_InCsMode(playState)) {
					} else if (YREG(15) == 0x10) {
						Audio_PlaySoundGeneral(
							NA_SE_SY_ERROR,
							&gSfxDefaultPos,
							4,
							&gSfxDefaultFreqAndVolScale,
							&gSfxDefaultFreqAndVolScale,
							&gSfxDefaultReverb
						);
					} else {
						func_800BC490(playState, playState->unk_1242B ^ 3);
					}
				}
				func_800BC450(playState);
			}
			
			SkyboxDraw_Update(&playState->skyboxCtx);
			
			if ((playState->pauseCtx.state != 0) || (playState->pauseCtx.debugState != 0)) {
				KaleidoScopeCall_Update(playState);
			} else if (playState->gameOverCtx.state != GAMEOVER_INACTIVE) {
				GameOver_Update(playState);
			} else {
				Message_Update(playState);
			}
			
			Interface_Update(playState);
			AnimationContext_Update(playState, &playState->animationCtx);
			SoundSource_UpdateAll(playState);
			ShrinkWindow_Update(R_UPDATE_RATE);
			TransitionFade_Update(&playState->transitionFade, R_UPDATE_RATE);
		} else {
			goto skip;
		}
	}
skip:
	
	if ((sp80 == 0) || (gDbgCamEnabled)) {
		s32 i;
		
		playState->nextCamId = playState->activeCamId;
		
		for (i = 0; i < NUM_CAMS; i++) {
			if ((i != playState->nextCamId) && (playState->cameraPtrs[i] != NULL)) {
				Camera_Update(playState->cameraPtrs[i]);
			}
		}
		
		Camera_Update(playState->cameraPtrs[playState->nextCamId]);
	}
	
	Environment_Update(
		playState,
		&playState->envCtx,
		&playState->lightCtx,
		&playState->pauseCtx,
		&playState->msgCtx,
		&playState->gameOverCtx,
		playState->state.gfxCtx
	);
	
	// if (neEnIn != playState->nextEntranceIndex)
	// 	gExitParam.lower = playState->nextEntranceIndex;
}

void NewPlay_Main(PlayState* playState) {
	D_8012D1F8 = &playState->state.input[0];
	
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
		Play_Update(playState);
	Play_Draw(playState);
	
#ifdef DEV_BUILD
	DebugMenu_Update(playState);
#endif
}

void NewPlay_SetFadeOut(PlayState* play) {
	s16 entranceIndex;
	
	if (gExitParam.isExit == false) {
		if (!IS_DAY) {
			if (!LINK_IS_ADULT) {
				entranceIndex = play->nextEntranceIndex + 1;
			} else {
				entranceIndex = play->nextEntranceIndex + 3;
			}
		} else {
			if (!LINK_IS_ADULT) {
				entranceIndex = play->nextEntranceIndex;
			} else {
				entranceIndex = play->nextEntranceIndex + 2;
			}
		}
		
		play->transitionType = gEntranceTable[entranceIndex].field & 0x7F;
	} else {
		play->transitionType = gExitParam.exit.fadeOut;
	}
}

void NewPlay_SetupRespawn(PlayState* this, s32 respawnMode, s32 playerParams) {
	RespawnData* respawnData = &gSaveContext.respawn[respawnMode];
	Player* player = GET_PLAYER(this);
	s32 entranceIndex = gSaveContext.entranceIndex;
	s8 roomIndex = this->roomCtx.curRoom.num;
	
	MemCpy(&gExitParam.respawn[respawnMode], &gExitParam.exit, sizeof(NewExit));
	
	respawnData->entranceIndex = entranceIndex;
	respawnData->roomIndex = roomIndex;
	respawnData->pos = player->actor.world.pos;
	respawnData->yaw = player->actor.shape.rot.y;
	respawnData->playerParams = playerParams;
	respawnData->tempSwchFlags = this->actorCtx.flags.tempSwch;
	respawnData->tempCollectFlags = this->actorCtx.flags.tempCollect;
}