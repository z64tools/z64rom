/*
 * File: z_select.c
 * Overlay: ovl_select
 * Description: Debug Scene Select Menu
 */

#include <uLib.h>
#include "vt.h"
#include "alloca.h"
#include <asm_macros.h>

Asm_SymbolAlias("__z64_init", Select_Init);
Asm_SymbolAlias("__z64_dest", Select_Destroy);

asm ("Select_LoadTitle = 0x80800B90");
asm ("Title_Init = 0x80800878");

#define COMMENT 0
#define SETTER  1

f32 fmodf(f32, f32);

void Select_SetupColor(GfxPrint* printer, u32 id) {
	switch (id) {
		case COMMENT:
			GfxPrint_SetColor(printer, 75, 75, 75, 255);
			break;
		case SETTER:
			GfxPrint_SetColor(printer, 255, 255, 255, 255);
			break;
	}
}

void Select_LoadTitle(SelectContext* this) {
	this->state.running = false;
	SET_NEXT_GAMESTATE(&this->state, Title_Init, TitleContext);
}

void Select_LoadGame(SelectContext* this, s32 entranceIndex) {
	if (gSaveContext.fileNum == 0xFF) {
		Sram_InitDebugSave();
		// Set the fill target to be the saved magic amount
		gSaveContext.magicFillTarget = gSaveContext.magic;
		// Set `magicLevel` and `magic` to 0 so `magicCapacity` then `magic` grows from nothing
		// to respectively the full capacity and `magicFillTarget`
		gSaveContext.magicCapacity = 0;
		gSaveContext.magicLevel = gSaveContext.magic = 0;
	}
	gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
		gSaveContext.buttonStatus[3] = gSaveContext.buttonStatus[4] = BTN_ENABLED;
	gSaveContext.unk_13E7 = gSaveContext.unk_13E8 = gSaveContext.unk_13EA = gSaveContext.unk_13EC = 0;
	Audio_QueueSeqCmd(SEQ_PLAYER_BGM_MAIN << 24 | NA_BGM_STOP);
	gSaveContext.entranceIndex = entranceIndex;
	gSaveContext.respawnFlag = 0;
	gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
	gSaveContext.seqId = (u8)NA_BGM_DISABLED;
	gSaveContext.natureAmbienceId = 0xFF;
	gSaveContext.showTitleCard = true;
	gWeatherMode = WEATHER_MODE_CLEAR;
	this->state.running = false;
	SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);
}

// "Translation" (Actual name)
static SceneSelectEntry sScenes[] = {
#include "LevelNames.h"
	{ "Boot Title", (void*)Select_LoadTitle, 0 },
};

void Select_UpdateMenu(SelectContext* this) {
	Input* input = &this->state.input[0];
	SceneSelectEntry* selectedScene;
	
	if (this->verticalInputAccumulator == 0) {
		if (CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_START)) {
			selectedScene = &this->scenes[this->currentScene];
			if (selectedScene->loadFunc != NULL) {
				selectedScene->loadFunc(this, selectedScene->entranceIndex);
			}
		}
		
		if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
			if (LINK_AGE_IN_YEARS == YEARS_ADULT) {
				gSaveContext.linkAge = LINK_AGE_CHILD;
			} else {
				gSaveContext.linkAge = LINK_AGE_ADULT;
			}
		}
		
		if (CHECK_BTN_ALL(input->press.button, BTN_R)) {
			if (gSaveContext.cutsceneIndex <= 0x8000) {
				gSaveContext.cutsceneIndex = 0xFFF0;
			} else if (gSaveContext.cutsceneIndex == 0xFFF0) {
				gSaveContext.cutsceneIndex = 0xFFF1;
			} else if (gSaveContext.cutsceneIndex == 0xFFF1) {
				gSaveContext.cutsceneIndex = 0xFFF2;
			} else if (gSaveContext.cutsceneIndex == 0xFFF2) {
				gSaveContext.cutsceneIndex = 0xFFF3;
			} else if (gSaveContext.cutsceneIndex == 0xFFF3) {
				gSaveContext.cutsceneIndex = 0xFFF4;
			} else if (gSaveContext.cutsceneIndex == 0xFFF4) {
				gSaveContext.cutsceneIndex = 0xFFF5;
			} else if (gSaveContext.cutsceneIndex == 0xFFF5) {
				gSaveContext.cutsceneIndex = 0xFFF6;
			} else if (gSaveContext.cutsceneIndex == 0xFFF6) {
				gSaveContext.cutsceneIndex = 0xFFF7;
			} else if (gSaveContext.cutsceneIndex == 0xFFF7) {
				gSaveContext.cutsceneIndex = 0xFFF8;
			} else if (gSaveContext.cutsceneIndex == 0xFFF8) {
				gSaveContext.cutsceneIndex = 0xFFF9;
			} else if (gSaveContext.cutsceneIndex == 0xFFF9) {
				gSaveContext.cutsceneIndex = 0xFFFA;
			} else if (gSaveContext.cutsceneIndex == 0xFFFA) {
				gSaveContext.cutsceneIndex = 0x8000;
			}
		} else if (CHECK_BTN_ALL(input->press.button, BTN_Z)) {
			if (gSaveContext.cutsceneIndex <= 0x8000) {
				gSaveContext.cutsceneIndex = 0xFFFA;
			} else if (gSaveContext.cutsceneIndex == 0xFFF0) {
				gSaveContext.cutsceneIndex = 0x8000;
			} else if (gSaveContext.cutsceneIndex == 0xFFF1) {
				gSaveContext.cutsceneIndex = 0xFFF0;
			} else if (gSaveContext.cutsceneIndex == 0xFFF2) {
				gSaveContext.cutsceneIndex = 0xFFF1;
			} else if (gSaveContext.cutsceneIndex == 0xFFF3) {
				gSaveContext.cutsceneIndex = 0xFFF2;
			} else if (gSaveContext.cutsceneIndex == 0xFFF4) {
				gSaveContext.cutsceneIndex = 0xFFF3;
			} else if (gSaveContext.cutsceneIndex == 0xFFF5) {
				gSaveContext.cutsceneIndex = 0xFFF4;
			} else if (gSaveContext.cutsceneIndex == 0xFFF6) {
				gSaveContext.cutsceneIndex = 0xFFF5;
			} else if (gSaveContext.cutsceneIndex == 0xFFF7) {
				gSaveContext.cutsceneIndex = 0xFFF6;
			} else if (gSaveContext.cutsceneIndex == 0xFFF8) {
				gSaveContext.cutsceneIndex = 0xFFF7;
			} else if (gSaveContext.cutsceneIndex == 0xFFF9) {
				gSaveContext.cutsceneIndex = 0xFFF8;
			} else if (gSaveContext.cutsceneIndex == 0xFFFA) {
				gSaveContext.cutsceneIndex = 0xFFF9;
			}
		}
		
		gSaveContext.nightFlag = 0;
		if (gSaveContext.cutsceneIndex == 0) {
			gSaveContext.nightFlag = 1;
		}
		
		if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
			if (this->lockUp == true) {
				this->timerUp = 0;
			}
			if (this->timerUp == 0) {
				this->timerUp = 20;
				this->lockUp = true;
				Audio_PlaySoundGeneral(
					NA_SE_IT_SWORD_IMPACT,
					&gSfxDefaultPos,
					4,
					&gSfxDefaultFreqAndVolScale,
					&gSfxDefaultFreqAndVolScale,
					&gSfxDefaultReverb
				);
				this->verticalInput = R_UPDATE_RATE;
			}
		}
		
		if (CHECK_BTN_ALL(input->cur.button, BTN_DUP) && this->timerUp == 0) {
			Audio_PlaySoundGeneral(
				NA_SE_IT_SWORD_IMPACT,
				&gSfxDefaultPos,
				4,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultReverb
			);
			this->verticalInput = R_UPDATE_RATE * 3;
		}
		
		if (CHECK_BTN_ALL(input->press.button, BTN_DDOWN)) {
			if (this->lockDown == true) {
				this->timerDown = 0;
			}
			if (this->timerDown == 0) {
				this->timerDown = 20;
				this->lockDown = true;
				Audio_PlaySoundGeneral(
					NA_SE_IT_SWORD_IMPACT,
					&gSfxDefaultPos,
					4,
					&gSfxDefaultFreqAndVolScale,
					&gSfxDefaultFreqAndVolScale,
					&gSfxDefaultReverb
				);
				this->verticalInput = -R_UPDATE_RATE;
			}
		}
		
		if (CHECK_BTN_ALL(input->cur.button, BTN_DDOWN) && (this->timerDown == 0)) {
			Audio_PlaySoundGeneral(
				NA_SE_IT_SWORD_IMPACT,
				&gSfxDefaultPos,
				4,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultReverb
			);
			this->verticalInput = -R_UPDATE_RATE * 3;
		}
		
		if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT) || CHECK_BTN_ALL(input->cur.button, BTN_DLEFT)) {
			Audio_PlaySoundGeneral(
				NA_SE_IT_SWORD_IMPACT,
				&gSfxDefaultPos,
				4,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultReverb
			);
			this->verticalInput = R_UPDATE_RATE;
		}
		
		if (CHECK_BTN_ALL(input->press.button, BTN_DRIGHT) || CHECK_BTN_ALL(input->cur.button, BTN_DRIGHT)) {
			Audio_PlaySoundGeneral(
				NA_SE_IT_SWORD_IMPACT,
				&gSfxDefaultPos,
				4,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultFreqAndVolScale,
				&gSfxDefaultReverb
			);
			this->verticalInput = -R_UPDATE_RATE;
		}
	}
	
	if (CHECK_BTN_ALL(input->press.button, BTN_L)) {
		this->pageDownIndex++;
		this->pageDownIndex =
			(this->pageDownIndex + ARRAY_COUNT(this->pageDownStops)) % ARRAY_COUNT(this->pageDownStops);
		this->currentScene = this->topDisplayedScene = this->pageDownStops[this->pageDownIndex];
	}
	
	this->verticalInputAccumulator += this->verticalInput;
	
	if (this->verticalInputAccumulator < -7) {
		this->verticalInput = 0;
		this->verticalInputAccumulator = 0;
		
		this->currentScene++;
		this->currentScene = (this->currentScene + this->count) % this->count;
		
		if (this->currentScene == ((this->topDisplayedScene + this->count + 19) % this->count)) {
			this->topDisplayedScene++;
			this->topDisplayedScene = (this->topDisplayedScene + this->count) % this->count;
		}
	}
	
	if (this->verticalInputAccumulator > 7) {
		this->verticalInput = 0;
		this->verticalInputAccumulator = 0;
		
		if (this->currentScene == this->topDisplayedScene) {
			this->topDisplayedScene -= 2;
			this->topDisplayedScene = (this->topDisplayedScene + this->count) % this->count;
		}
		
		this->currentScene--;
		this->currentScene = (this->currentScene + this->count) % this->count;
		
		if (this->currentScene == ((this->topDisplayedScene + this->count) % this->count)) {
			this->topDisplayedScene--;
			this->topDisplayedScene = (this->topDisplayedScene + this->count) % this->count;
		}
	}
	
	this->currentScene = (this->currentScene + this->count) % this->count;
	this->topDisplayedScene = (this->topDisplayedScene + this->count) % this->count;
	
	dREG(80) = this->currentScene;
	dREG(81) = this->topDisplayedScene;
	dREG(82) = this->pageDownIndex;
	
	if (this->timerUp != 0) {
		this->timerUp--;
	}
	
	if (this->timerUp == 0) {
		this->lockUp = false;
	}
	
	if (this->timerDown != 0) {
		this->timerDown--;
	}
	
	if (this->timerDown == 0) {
		this->lockDown = false;
	}
}

void Select_PrintMenu(SelectContext* this, GfxPrint* printer) {
	s32 scene;
	s32 i;
	char* name;
	
	GfxPrint_SetColor(printer, 175, 125, 255, 255);
	GfxPrint_SetPos(printer, 10, 2);
	GfxPrint_Printf(printer, "Z64ROM Level Select");
	
	for (i = 0; i < 20; i++) {
		Color_HSL color;
		Color_RGB8 rgb;
		
		scene = (this->topDisplayedScene + i + this->count) % this->count;
		
		color.h = (f32)sScenes[scene].entranceIndex / __INT8_MAX__;
		color.s = 0.3f;
		color.l = 0.5f;
		
		color.h = fmodf(color.h, 1.0f);
		
		if (scene == this->currentScene) {
			color.s = 0.75f;
			color.l = 0.9f;
		}
		
		Color_ToRGB(&rgb, &color);
		GfxPrint_SetPos(printer, 1, i + 4);
		GfxPrint_SetColor(printer, rgb.r, rgb.g, rgb.b, 255);
		
		name = this->scenes[scene].name;
		if (name == NULL) {
			name = "**Null**";
		}
		
		GfxPrint_Printf(printer, "%-3d %s", scene, name);
	}
}

static const char* sLoadingMessages[] = {
	"Please wait a minute",
	"Hold on a sec",
	"Wait a moment",
	"Loading",
	"Now working",
	"Now creating",
	"It's not broken",
	"Coffee Break",
	"Please set B side",
	"Be patient, now",
	"Please wait just a minute",
	"Don't worry, don't worry. Take a break, take a break.",
};

void Select_PrintLoadingMessage(SelectContext* this, GfxPrint* printer) {
	s32 randomMsg;
	
	GfxPrint_SetPos(printer, 10, 15);
	GfxPrint_SetColor(printer, 255, 255, 255, 255);
	randomMsg = Rand_ZeroOne() * ARRAY_COUNT(sLoadingMessages);
	GfxPrint_Printf(printer, "%s", sLoadingMessages[randomMsg]);
}

static const char* sAgeLabels[] = {
	"Young",               // "17(young)"
	"Child",               // "5(very young)"
};

void Select_PrintAgeSetting(SelectContext* this, GfxPrint* printer, s32 age) {
	GfxPrint_SetPos(printer, 2, 26);
	Select_SetupColor(printer, SETTER);
	GfxPrint_Printf(printer, "Age (B):     %s", sAgeLabels[age]);
}

void Select_PrintCutsceneSetting(SelectContext* this, GfxPrint* printer, u16 csIndex) {
	static s8 timeH = 12;
	static s8 timeM = 0;
	char timeBuffer[64];
	char* label;
	
	GfxPrint_SetPos(printer, 2, 25);
	Select_SetupColor(printer, SETTER);
	
	switch (csIndex) {
		case 0x0000 ... 0x8000:
			if (CHK_ANY(press, BTN_CUP))
				timeH++;
			if (CHK_ANY(press, BTN_CDOWN))
				timeH--;
			if (CHK_ANY(press, BTN_CRIGHT))
				timeM += 10;
			if (CHK_ANY(press, BTN_CLEFT))
				timeM -= 10;
			
			if (timeM < 0) {
				timeH--;
				timeM = 50;
			} else if (timeM >= 60) {
				timeH++;
				timeM = 0;
			}
			
			if (timeH < 0) {
				timeH = 23;
			} else if (timeH >= 24) {
				timeH = 0;
			}
			
			gSaveContext.dayTime = CLOCK_TIME(timeH, timeM + 0.5f);
			label = timeBuffer;
			sprintf(timeBuffer, "%02d.%02d%s", timeH % 12, timeM, timeH >= 12 ? "pm" : "am");
			break;
		case 0xFFF0:
			gSaveContext.dayTime = CLOCK_TIME(12, 0);
			label = "Cutscene 0";
			break;
		case 0xFFF1:
			label = "Cutscene 1";
			break;
		case 0xFFF2:
			label = "Cutscene 2";
			break;
		case 0xFFF3:
			label = "Cutscene 3";
			break;
		case 0xFFF4:
			label = "Cutscene 4";
			break;
		case 0xFFF5:
			label = "Cutscene 5";
			break;
		case 0xFFF6:
			label = "Cutscene 6";
			break;
		case 0xFFF7:
			label = "Cutscene 7";
			break;
		case 0xFFF8:
			label = "Cutscene 8";
			break;
		case 0xFFF9:
			label = "Cutscene 9";
			break;
		case 0xFFFA:
			label = "Cutscene 10";
			break;
	}
	
	gSaveContext.skyboxTime = gSaveContext.dayTime;
	GfxPrint_Printf(printer, "Setup (Z/R): %s", label);
	
	if (csIndex == 0x8000) {
		GfxPrint_SetPos(printer, 22, 25);
		Select_SetupColor(printer, COMMENT);
		GfxPrint_Printf(printer, "(C, hour/minute)", label);
	}
}

void Select_DrawMenu(SelectContext* this) {
	GraphicsContext* gfxCtx = this->state.gfxCtx;
	GfxPrint* printer;
	
	OPEN_DISPS(gfxCtx, "../z_select.c", 930);
	
	gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
	func_80095248(gfxCtx, 0, 0, 0);
	SET_FULLSCREEN_VIEWPORT(&this->view);
	View_Apply(&this->view, VIEW_ALL);
	Gfx_SetupDL_28Opa(gfxCtx);
	
	printer = alloca(sizeof(GfxPrint));
	GfxPrint_Init(printer);
	GfxPrint_Open(printer, POLY_OPA_DISP);
	Select_PrintMenu(this, printer);
	Select_PrintAgeSetting(this, printer, ((void)0, gSaveContext.linkAge));
	Select_PrintCutsceneSetting(this, printer, ((void)0, gSaveContext.cutsceneIndex));
	POLY_OPA_DISP = GfxPrint_Close(printer);
	GfxPrint_Destroy(printer);
	
	CLOSE_DISPS(gfxCtx, "../z_select.c", 966);
}

void Select_DrawLoadingScreen(SelectContext* this) {
	GraphicsContext* gfxCtx = this->state.gfxCtx;
	GfxPrint* printer;
	
	OPEN_DISPS(gfxCtx, "../z_select.c", 977);
	
	gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
	func_80095248(gfxCtx, 0, 0, 0);
	SET_FULLSCREEN_VIEWPORT(&this->view);
	View_Apply(&this->view, VIEW_ALL);
	Gfx_SetupDL_28Opa(gfxCtx);
	
	printer = alloca(sizeof(GfxPrint));
	GfxPrint_Init(printer);
	GfxPrint_Open(printer, POLY_OPA_DISP);
	Select_PrintLoadingMessage(this, printer);
	POLY_OPA_DISP = GfxPrint_Close(printer);
	GfxPrint_Destroy(printer);
	
	CLOSE_DISPS(gfxCtx, "../z_select.c", 1006);
}

void Select_Draw(SelectContext* this) {
	GraphicsContext* gfxCtx = this->state.gfxCtx;
	
	OPEN_DISPS(gfxCtx, "../z_select.c", 1013);
	
	gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
	func_80095248(gfxCtx, 0, 0, 0);
	SET_FULLSCREEN_VIEWPORT(&this->view);
	View_Apply(&this->view, VIEW_ALL);
	
	if (!this->state.running) {
		Select_DrawLoadingScreen(this);
	} else {
		Select_DrawMenu(this);
	}
	
	CLOSE_DISPS(gfxCtx, "../z_select.c", 1037);
}

void Select_Main(GameState* thisx) {
	SelectContext* this = (SelectContext*)thisx;
	
	Select_UpdateMenu(this);
	Select_Draw(this);
}

void Select_Destroy(GameState* thisx) {
}

void Select_Init(GameState* thisx) {
	SelectContext* this = (SelectContext*)thisx;
	u32 size;
	
	this->state.main = Select_Main;
	this->state.destroy = Select_Destroy;
	this->scenes = sScenes;
	this->topDisplayedScene = 0;
	this->currentScene = 0;
	this->pageDownStops[0] = 0; // Hyrule Field
	this->pageDownStops[1] = 19; // Temple Of Time
	this->pageDownStops[2] = 37; // Treasure Chest Game
	this->pageDownStops[3] = 51; // Gravekeeper's Hut
	this->pageDownStops[4] = 59; // Zora Shop
	this->pageDownStops[5] = 73; // Bottom of the Well
	this->pageDownStops[6] = 91; // Escaping Ganon's Tower 3
	this->pageDownIndex = 0;
	this->opt = 0;
	this->count = ARRAY_COUNT(sScenes);
	View_Init(&this->view, this->state.gfxCtx);
	this->view.flags = (VIEW_PROJECTION_ORTHO | VIEW_VIEWPORT);
	this->verticalInputAccumulator = 0;
	this->verticalInput = 0;
	this->timerUp = 0;
	this->timerDown = 0;
	this->lockUp = 0;
	this->lockDown = 0;
	this->unk_234 = 0;
	
	size = gDmaDataTable[937].vromEnd - gDmaDataTable[937].vromStart;
	
	if ((dREG(80) >= 0) && (dREG(80) < this->count)) {
		this->currentScene = dREG(80);
		this->topDisplayedScene = dREG(81);
		this->pageDownIndex = dREG(82);
	}
	R_UPDATE_RATE = 1;
	
	this->staticSegment = GameState_Alloc(&this->state, size, "../z_select.c", 1114);
	DmaMgr_SendRequest1(this->staticSegment, gDmaDataTable[937].vromStart, size, "../z_select.c", 1115);
	gSaveContext.cutsceneIndex = 0x8000;
	gSaveContext.linkAge = LINK_AGE_CHILD;
}
