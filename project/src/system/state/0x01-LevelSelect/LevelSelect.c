/*
 * File: z_select.c
 * Overlay: ovl_select
 * Description: Debug Scene Select Menu
 */

#include <uLib.h>
#include "vt.h"
#include "alloca.h"
#include <asm_macros.h>

Asm_SymbolAlias("__z64_init", MapSelect_Init);
Asm_SymbolAlias("__z64_dest", Select_Destroy);

asm ("ConsoleLogo_Init = 0x80800878");

#define COMMENT 0
#define SETTER  1

typedef struct {
    u8    id;
    char* name;
} Level;

Level* gLevel;
char* gScene;
u8 gSpawn;

static Time sTime;
static s8 sInitTime;

void Select_SetupColor(GfxPrint* printer, u32 id) {
    switch (id) {
        case COMMENT:
            GfxPrint_SetColor(printer, 75, 75, 75, 255);
            break;
        case SETTER:
            GfxPrint_SetColor(printer, 175, 175, 175, 255);
            break;
    }
}

void Select_LoadTitle(MapSelectState* this) {
    this->state.running = false;
    SET_NEXT_GAMESTATE(&this->state, ConsoleLogo_Init, ConsoleLogoState);
}

void Select_LoadGame(MapSelectState* this, u8 sceneIndex) {
    gExitParam.nextEntranceIndex = 0x8000;
    gExitParam.exit = (NewExit) {
        .sceneIndex = sceneIndex,
        .headerIndex = 0xF,
        .spawnIndex = gSpawn,
        .fadeIn = 2,
        .fadeOut = 2,
    };
    
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
    gSaveContext.entranceIndex = 0x8000;
    gSaveContext.respawnFlag = 0;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = 0xFF;
    gSaveContext.showTitleCard = true;
    gWeatherMode = WEATHER_MODE_CLEAR;
    this->state.running = false;
    SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);
}

void Select_UpdateMenu(MapSelectState* this) {
    Input* input = &this->state.input[0];
    
    if (this->verticalInputAccumulator == 0) {
        if (CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_START)) {
            Select_LoadGame(this, gLevel[this->currentScene].id);
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
                Audio_PlaySfxGeneral(
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
            Audio_PlaySfxGeneral(
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
                Audio_PlaySfxGeneral(
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
            Audio_PlaySfxGeneral(
                NA_SE_IT_SWORD_IMPACT,
                &gSfxDefaultPos,
                4,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultReverb
            );
            this->verticalInput = -R_UPDATE_RATE * 3;
        }
        
        if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT)) {
            Audio_PlaySfxGeneral(
                NA_SE_SY_CURSOR,
                &gSfxDefaultPos,
                4,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultReverb
            );
            gSpawn--;
        }
        
        if (CHECK_BTN_ALL(input->press.button, BTN_DRIGHT)) {
            Audio_PlaySfxGeneral(
                NA_SE_SY_CURSOR,
                &gSfxDefaultPos,
                4,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultReverb
            );
            gSpawn++;
        }
        
        gSpawn = gSpawn & 0x1F;
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
    if (this->count <= 20)
        this->topDisplayedScene = 0;
    
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

void Select_PrintMenu(MapSelectState* this, GfxPrint* printer) {
    s32 scene;
    s32 i;
    char* name;
    static s32 timer;
    
    printer->flags |= GFXP_FLAG_SHADOW;
    
    GfxPrint_SetColor(printer, 175, 125, 255, 255);
    GfxPrint_SetPos(printer, 10, 2);
    GfxPrint_Printf(printer, "Z64ROM Level Select");
    
    for (i = 0; i < CLAMP_MAX(this->count, 20); i++) {
        Color_HSL color;
        Color_RGB8 rgb;
        
        scene = (this->topDisplayedScene + i + this->count) % this->count;
        
        color.h = 0.5f;
        color.s = 0.01f;
        color.l = 0.7f;
        
        GfxPrint_SetPos(printer, 1, i + 4);
        
        if (scene == this->currentScene) {
            color.l = 0.5f + ABS(Math_SinS(timer) * 0.1);
            color.s = 0.8;
            color.h = 0.02;
        }
        
        if (name == NULL) {
            name = "**Null**";
        }
        
        GfxPrint_Printf(printer, "", gLevel[scene].id);
        
        rgb = Color_HslToRgb(color.h, color.s, color.l);
        GfxPrint_SetColor(printer, 100, 100, 100, 255);
        GfxPrint_Printf(printer, "%02X ", gLevel[scene].id);
        GfxPrint_SetColor(printer, rgb.r, rgb.g, rgb.b, 255);
        GfxPrint_Printf(printer, "%s", gLevel[scene].name);
    }
    
    timer += DEG_TO_BINANG(1.76);
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

void Select_PrintLoadingMessage(MapSelectState* this, GfxPrint* printer) {
    s32 randomMsg;
    
    GfxPrint_SetPos(printer, 10, 15);
    GfxPrint_SetColor(printer, 255, 255, 255, 255);
    randomMsg = Rand_ZeroOne() * ARRAY_COUNT(sLoadingMessages);
    GfxPrint_Printf(printer, "%s", sLoadingMessages[randomMsg]);
}

void Select_PrintAgeSetting(MapSelectState* this, GfxPrint* printer, s32 age) {
    Select_SetupColor(printer, SETTER);
    GfxPrint_SetPos(printer, 4, 26);
    GfxPrint_Printf(printer, "Age (B):      ");
    
    if (age == 0) {
        GfxPrint_SetColor(printer, 255, 75, 75, 255);
        GfxPrint_Printf(printer, "Adult");
    } else {
        GfxPrint_SetColor(printer, 52, 155, 235, 255);
        GfxPrint_Printf(printer, "Child");
    }
    
    Select_SetupColor(printer, SETTER);
    GfxPrint_SetPos(printer, 4, 27);
    GfxPrint_Printf(printer, "Spawn (D-LR): ");
    
    GfxPrint_SetColor(printer, 125, 52, 235, 255);
    GfxPrint_Printf(printer, "%d", gSpawn);
}

void Select_PrintCutsceneSetting(MapSelectState* this, GfxPrint* printer, u16 csIndex) {
    char timeBuffer[64];
    char* label;
    Color_RGB8 rgb;
    
    if (sInitTime == false) {
        sTime = Play_GetTime();
        sInitTime = true;
    }
    
    GfxPrint_SetPos(printer, 4, 25);
    Select_SetupColor(printer, SETTER);
    GfxPrint_Printf(printer, "Setup (Z/R):  ");
    
    GfxPrint_SetColor(printer, 255, 165, 75, 255);
    
    switch (csIndex) {
        case 0x0000 ... 0x8000:
            if (CHK_ANY(press, BTN_CUP))
                sTime.hour = WrapS(sTime.hour + 1, 0, 24);
            if (CHK_ANY(press, BTN_CDOWN))
                sTime.hour = WrapS(sTime.hour - 1, 0, 24);
            if (CHK_ANY(press, BTN_CRIGHT))
                sTime.minute = WrapS(sTime.minute + 1, 0, 60);
            if (CHK_ANY(press, BTN_CLEFT))
                sTime.minute = WrapS(sTime.minute - 1, 0, 60);
            
            gSaveContext.dayTime = CLOCK_TIME(sTime.hour, sTime.minute + 0.5f);
            label = timeBuffer;
            sprintf(timeBuffer, "%02d.%02d%s", sTime.hour % 12, sTime.minute, sTime.hour >= 12 ? "pm" : "am");
            
            rgb = Color_HslToRgb(
                (u16)(gSaveContext.dayTime + 0x8000) / (f32)__UINT16_MAX__,
                0.45f,
                0.5f
            );
            GfxPrint_SetColor(printer, rgb.r, rgb.g, rgb.b, 255);
            
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
    GfxPrint_Printf(printer, "%s", label);
}

static void Select_Rectangle(f32 x, f32 y, f32 w, f32 h, Color_RGB8 rgb) {
    x *= 16.0f;
    y *= 16.0f;
    w *= 16.0f;
    h *= 16.0f;
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetOtherMode(
        POLY_OPA_DISP++,
        G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TL_TILE |
        G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
        G_AC_NONE | G_ZS_PRIM | G_RM_OPA_SURF | G_RM_OPA_SURF2
    );
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, rgb.r, rgb.g, rgb.b, 0xFF);
    
    //crustify
	gSPTextureRectangle(
		POLY_OPA_DISP++,
		((s32)(x) << 1),
		((s32)(y) << 1),
		((s32)(x + w) << 1),
		((s32)(y + h) << 1),
		G_TX_RENDERTILE,
		0, 0, 0, 0
	);
    //uncrustify
    gDPPipeSync(POLY_OPA_DISP++);
}

void Select_DrawMenu(MapSelectState* this) {
    GraphicsContext* gfxCtx = this->state.gfxCtx;
    GfxPrint* printer;
    
    OPEN_DISPS(gfxCtx, "../z_select.c", 930);
    
    gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
    Gfx_SetupFrame(gfxCtx, 0, 0, 0);
    SET_FULLSCREEN_VIEWPORT(&this->view);
    View_Apply(&this->view, VIEW_ALL);
    Gfx_SetupDL_28Opa(gfxCtx);
    
    Select_Rectangle(3.35f, 0, 0.12f, 28, Color_HslToRgb(0.0, 0.0, 0.7));
    
    Select_Rectangle(0, 24.5f, 48, 4, Color_HslToRgb(0.0, 0.0, 0.08));
    Select_Rectangle(0, 24.5f, 48, 0.08f, Color_HslToRgb(0.0, 0.0, 0.7));
    Select_Rectangle(0, 24.5f + 4.0f, 48, 0.08f, Color_HslToRgb(0.0, 0.0, 0.7));
    
    Select_Rectangle(0, 0, 48, 3.5f, Color_HslToRgb(0.0, 0.0, 0.08));
    Select_Rectangle(0, 3.5f, 48, 0.08f, Color_HslToRgb(0.0, 0.0, 0.7));
    
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

void Select_DrawLoadingScreen(MapSelectState* this) {
    GraphicsContext* gfxCtx = this->state.gfxCtx;
    GfxPrint* printer;
    
    OPEN_DISPS(gfxCtx, "../z_select.c", 977);
    
    gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
    Gfx_SetupFrame(gfxCtx, 0, 0, 0);
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

void Select_Draw(MapSelectState* this) {
    GraphicsContext* gfxCtx = this->state.gfxCtx;
    
    OPEN_DISPS(gfxCtx, "../z_select.c", 1013);
    
    gSPSegment(POLY_OPA_DISP++, 0x00, NULL);
    Gfx_SetupFrame(gfxCtx, 0, 0, 0);
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
    MapSelectState* this = (MapSelectState*)thisx;
    
    Select_UpdateMenu(this);
    Select_Draw(this);
}

#include <code/code_800EC960.h>
void Select_Destroy(GameState* thisx) {
    MapSelectState* this = (MapSelectState*)thisx;
    
    sAudioCutsceneFlag = false;
    GameAlloc_Free(&this->state.alloc, gLevel);
    GameAlloc_Free(&this->state.alloc, gScene);
}

static void Select_ParseLevelMapData(GameState* thisx) {
    MapSelectState* this = (MapSelectState*)thisx;
    u32 size = gDmaDataTable[4].vromEnd - gDmaDataTable[4].vromStart;
    char* scenTbl;
    
    this->count = 0;
    gScene = GameAlloc_Malloc(&this->state.alloc, size);
    DmaMgr_SendRequest0(gScene, gDmaDataTable[4].vromStart, size);
    
    scenTbl = gScene;
    for (s32 i = 0; scenTbl[0] != -1; i++) {
        this->count++;
        
        scenTbl++;
        scenTbl += strlen((char*)scenTbl) + 1;
        
        Assert(i < 0xFF);
    }
    
    gLevel = GameAlloc_Malloc(&this->state.alloc, sizeof(Level) * this->count);
    Assert(gLevel != NULL);
    
    scenTbl = gScene;
    for (s32 i = 0; scenTbl[0] != -1; i++) {
        gLevel[i].id = scenTbl[0];
        gLevel[i].name = (char*)scenTbl + 1;
        
        scenTbl++;
        scenTbl += strlen((char*)scenTbl) + 1;
    }
}

void MapSelect_Init(GameState* thisx) {
    MapSelectState* this = (MapSelectState*)thisx;
    u32 size = gDmaDataTable[4].vromEnd - gDmaDataTable[4].vromStart;
    
    Select_ParseLevelMapData(thisx);
    
    Audio_SetSequenceMode(0);
    this->state.main = Select_Main;
    this->state.destroy = Select_Destroy;
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
    DmaMgr_SendRequest0(this->staticSegment, gDmaDataTable[937].vromStart, size);
    gSaveContext.cutsceneIndex = 0x8000;
    gSaveContext.linkAge = LINK_AGE_CHILD;
}
