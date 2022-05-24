#include <uLib.h>
#include <vt.h>

/*
   z64ram = 0x800BD314
   z64rom = 0xB344B4
 */

#ifndef R_TRANS_DBG_ENABLED
typedef enum {
	/*  0 */ TRANS_MODE_OFF,
	/*  1 */ TRANS_MODE_SETUP,
	/*  2 */ TRANS_MODE_INSTANCE_INIT,
	/*  3 */ TRANS_MODE_INSTANCE_RUNNING,
	/*  4 */ TRANS_MODE_FILL_WHITE_INIT,
	/*  5 */ TRANS_MODE_FILL_IN,
	/*  6 */ TRANS_MODE_FILL_OUT,
	/*  7 */ TRANS_MODE_FILL_BROWN_INIT,
	/*  8 */ TRANS_MODE_08, // unused
	/*  9 */ TRANS_MODE_09, // unused
	/* 10 */ TRANS_MODE_INSTANT,
	/* 11 */ TRANS_MODE_INSTANCE_WAIT,
	/* 12 */ TRANS_MODE_SANDSTORM_INIT,
	/* 13 */ TRANS_MODE_SANDSTORM,
	/* 14 */ TRANS_MODE_SANDSTORM_END_INIT,
	/* 15 */ TRANS_MODE_SANDSTORM_END,
	/* 16 */ TRANS_MODE_CS_BLACK_FILL_INIT,
	/* 17 */ TRANS_MODE_CS_BLACK_FILL
} TransitionMode;

typedef enum {
	/*  0 */ TRANS_TYPE_WIPE,
	/*  1 */ TRANS_TYPE_TRIFORCE,
	/*  2 */ TRANS_TYPE_FADE_BLACK,
	/*  3 */ TRANS_TYPE_FADE_WHITE,
	/*  4 */ TRANS_TYPE_FADE_BLACK_FAST,
	/*  5 */ TRANS_TYPE_FADE_WHITE_FAST,
	/*  6 */ TRANS_TYPE_FADE_BLACK_SLOW,
	/*  7 */ TRANS_TYPE_FADE_WHITE_SLOW,
	/*  8 */ TRANS_TYPE_WIPE_FAST,
	/*  9 */ TRANS_TYPE_FILL_WHITE2,
	/* 10 */ TRANS_TYPE_FILL_WHITE,
	/* 11 */ TRANS_TYPE_INSTANT,
	/* 12 */ TRANS_TYPE_FILL_BROWN,
	/* 13 */ TRANS_TYPE_FADE_WHITE_CS_DELAYED,
	/* 14 */ TRANS_TYPE_SANDSTORM_PERSIST,
	/* 15 */ TRANS_TYPE_SANDSTORM_END,
	/* 16 */ TRANS_TYPE_CS_BLACK_FILL,
	/* 17 */ TRANS_TYPE_FADE_WHITE_INSTANT,
	/* 18 */ TRANS_TYPE_FADE_GREEN,
	/* 19 */ TRANS_TYPE_FADE_BLUE,
	// transition types 20 - 31 are unused
	// transition types 32 - 55 are constructed using the TRANS_TYPE_CIRCLE macro
	/* 56 */ TRANS_TYPE_MAX = 56
} TransitionType;

#define TRANS_TRIGGER_OFF   0 // transition is not active
#define TRANS_TRIGGER_START 20 // start transition (exiting an area)
#define TRANS_TRIGGER_END   -20 // transition is ending (arriving in a new area)

#define R_TRANS_DBG_ENABLED CREG(11)
#define R_TRANS_DBG_TYPE    CREG(12)

#define Gameplay_SetupTransition(global, transType) func_800BC5E0(global, transType)

typedef enum {
	/* 0 */ TCA_NORMAL,
	/* 1 */ TCA_WAVE,
	/* 2 */ TCA_RIPPLE,
	/* 3 */ TCA_STARBURST
} TransitionCircleAppearance;

typedef enum {
	/* 0 */ TCC_BLACK,
	/* 1 */ TCC_WHITE,
	/* 2 */ TCC_GRAY,
	/* 3 */ TCC_SPECIAL // color varies depending on appearance. unused and appears broken
} TransitionCircleColor;

typedef enum {
	/* 0 */ TCS_FAST,
	/* 1 */ TCS_SLOW
} TransitionCircleSpeed;

#define TC_SET_PARAMS (1 << 7)

#define TRANS_TYPE_CIRCLE(appearance, color, speed) ((1 << 5) | ((color & 3) << 3) | ((appearance & 3) << 1) | (speed & 1))

typedef enum {
	/* 0 */ SANDSTORM_OFF,
	/* 1 */ SANDSTORM_FILL,
	/* 2 */ SANDSTORM_UNFILL,
	/* 3 */ SANDSTORM_ACTIVE,
	/* 4 */ SANDSTORM_DISSIPATE
} SandstormState;

#endif

void Gameplay_Update(GlobalContext* globalCtx) {
	s32 pad1;
	s32 sp80;
	Input* input;
	u32 i;
	s32 pad2;
	
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
					
					if (!R_TRANS_DBG_ENABLED) {
						Gameplay_SetupTransition(globalCtx, globalCtx->fadeTransition);
					} else {
						Gameplay_SetupTransition(globalCtx, R_TRANS_DBG_TYPE);
					}
					
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
		s32 pad3[5];
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
