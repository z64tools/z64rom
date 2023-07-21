#include <uLib.h>
#include <code/z_demo.h>

typedef enum {
    TYPE_EVENT_CHK_INF,
    TYPE_ITEM_GET_INF,
    TYPE_EVENT_INF,
    TYPE_ITEM,
    TYPE_QUEST_ITEM,
    TYPE_SWITCH_FLAG,
} CmdType;

typedef struct {
    struct {
        u8 lastCmd : 1; // O***-****
        u8 set     : 1; // *O**-****
        u8 type    : 6; // **OO-OOOO
    };
    u8 flag;
} CmdFlag;

typedef struct {
    /* 0x00 */ u16     cmdId;
    /* 0x02 */ u16     __padding;
    /* 0x04 */ NewExit exitParam;
    /* 0x08 */ CmdFlag list[];
} CmdHeader;

#define CLEAR_ITEMGETINF(flag) (gSaveContext.itemGetInf[(flag) >> 4] &= ~(1 << ((flag) & 0xF)))

static void* CutsceneCmd_ExitParam(PlayState* play, void* ptr) {
    CmdHeader* cmd = ptr;
    
    play->transitionTrigger = TRANS_TRIGGER_START;
    gExitParam.nextEntranceIndex = cmd->exitParam.upper;
    gExitParam.exit = cmd->exitParam;
    
    for (s32 i = 0;; i++) {
        CmdType type = cmd->list[i].type;
        u16 flag = cmd->list[i].flag;
        s8 set = cmd->list[i].set;
        
        switch (type) {
            case TYPE_EVENT_CHK_INF:
                if (set)
                    SET_EVENTCHKINF(flag);
                else
                    CLEAR_EVENTCHKINF(flag);
                break;
                
            case TYPE_ITEM_GET_INF:
                if (set)
                    SET_ITEMGETINF(flag);
                else
                    CLEAR_ITEMGETINF(flag);
                break;
                
            case TYPE_EVENT_INF:
                if (set)
                    SET_EVENTINF(flag);
                else
                    CLEAR_EVENTINF(flag);
                break;
                
            case TYPE_ITEM:
                if (set)
                    Item_Give(play, flag);
                else
                    Inventory_DeleteItem(flag, gItemSlots[flag]);
                break;
                
            case TYPE_QUEST_ITEM:
                if (set)
                    gSaveContext.inventory.questItems |= (1 << flag);
                else
                    gSaveContext.inventory.questItems &= ~(1 << flag);
                break;
                
            case TYPE_SWITCH_FLAG:
                if (set)
                    Flags_SetSwitch(play, flag);
                else
                    Flags_UnsetSwitch(play, flag);
                break;
        }
        
        if (cmd->list[i].lastCmd == false)
            break;
    }
    
    return cmd + 1;
}

Asm_VanillaHook(Cutscene_ProcessCommands);
void Cutscene_ProcessCommands(PlayState* play, CutsceneContext* csCtx, u8* cutscenePtr) {
    s16 i;
    s32 totalEntries;
    s32 cmdType;
    s32 cmdEntries;
    CsCmdBase* cmd;
    s32 cutsceneEndFrame;
    s16 j;
    
    if (cutscenePtr == NULL) {
        osLibPrintf("MemCpy(%08X, %08X, 4)", &totalEntries, cutscenePtr, 4);
        csCtx->state = CS_STATE_UNSKIPPABLE_INIT;
        
        return;
    }
    MemCpy(&totalEntries, cutscenePtr, 4);
    cutscenePtr += 4;
    MemCpy(&cutsceneEndFrame, cutscenePtr, 4);
    cutscenePtr += 4;
    
    if ((cutsceneEndFrame < csCtx->frames) && (csCtx->state != CS_STATE_UNSKIPPABLE_EXEC)) {
        csCtx->state = CS_STATE_UNSKIPPABLE_INIT;
        
        return;
    }
    
#ifdef DEV_BUILD
    if (CHECK_BTN_ALL(play->state.input[0].press.button, BTN_DRIGHT)) {
        csCtx->state = CS_STATE_UNSKIPPABLE_INIT;
        
        return;
    }
#endif
    
    for (i = 0; i < totalEntries; i++) {
        MemCpy(&cmdType, cutscenePtr, 4);
        cutscenePtr += 4;
        
        if (cmdType == -1) {
            return;
        }
        
        switch (cmdType) {
            case CS_CMD_MISC:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    func_80064824(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_LIGHTING:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    Cutscene_Command_SetLighting(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_PLAYBGM:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    Cutscene_Command_PlayBGM(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_STOPBGM:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    Cutscene_Command_StopBGM(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_FADEBGM:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    Cutscene_Command_FadeBGM(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_09:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    Cutscene_Command_09(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0xC;
                }
                break;
            case CS_CMD_SETTIME:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    func_80065134(play, csCtx, (void*)cutscenePtr);
                    cutscenePtr += 0xC;
                }
                break;
            case CS_CMD_SET_PLAYER_ACTION:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->linkAction = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_1:
            case 17:
            case 18:
            case 23:
            case 34:
            case 39:
            case 46:
            case 76:
            case 85:
            case 93:
            case 105:
            case 107:
            case 110:
            case 119:
            case 123:
            case 138:
            case 139:
            case 144:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[0] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_2:
            case 16:
            case 24:
            case 35:
            case 40:
            case 48:
            case 64:
            case 68:
            case 70:
            case 78:
            case 80:
            case 94:
            case 116:
            case 118:
            case 120:
            case 125:
            case 131:
            case 141:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[1] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_3:
            case 36:
            case 41:
            case 50:
            case 67:
            case 69:
            case 72:
            case 74:
            case 81:
            case 106:
            case 117:
            case 121:
            case 126:
            case 132:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[2] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_4:
            case 37:
            case 42:
            case 51:
            case 53:
            case 63:
            case 65:
            case 66:
            case 75:
            case 82:
            case 108:
            case 127:
            case 133:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[3] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_5:
            case 38:
            case 43:
            case 47:
            case 54:
            case 79:
            case 83:
            case 128:
            case 135:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[4] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_6:
            case 55:
            case 77:
            case 84:
            case 90:
            case 129:
            case 136:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[5] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_7:
            case 52:
            case 57:
            case 58:
            case 88:
            case 115:
            case 130:
            case 137:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[6] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_8:
            case 60:
            case 89:
            case 111:
            case 114:
            case 134:
            case 142:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[7] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_9:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[8] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_SET_ACTOR_ACTION_10:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if ((cmd->startFrame < csCtx->frames) && (csCtx->frames <= cmd->endFrame)) {
                        csCtx->npcActions[9] = (void*)cutscenePtr;
                    }
                    cutscenePtr += 0x30;
                }
                break;
            case CS_CMD_CAM_EYE:
                cutscenePtr += Cutscene_Command_CameraEyePoints(play, csCtx, (void*)cutscenePtr, 0);
                break;
            case CS_CMD_CAM_EYE_REL_TO_PLAYER:
                cutscenePtr += Cutscene_Command_CameraEyePoints(play, csCtx, (void*)cutscenePtr, 1);
                break;
            case CS_CMD_CAM_AT:
                cutscenePtr += Cutscene_Command_CameraLookAtPoints(play, csCtx, (void*)cutscenePtr, 0);
                break;
            case CS_CMD_CAM_AT_REL_TO_PLAYER:
                cutscenePtr += Cutscene_Command_CameraLookAtPoints(play, csCtx, (void*)cutscenePtr, 1);
                break;
            case CS_CMD_07:
                cutscenePtr += Cutscene_Command_07(play, csCtx, (void*)cutscenePtr, 0);
                break;
            case CS_CMD_08:
                cutscenePtr += Cutscene_Command_08(play, csCtx, (void*)cutscenePtr, 0);
                break;
            case CS_CMD_TERMINATOR:
                cutscenePtr += 4;
                Cutscene_Command_Terminator(play, csCtx, (void*)cutscenePtr);
                cutscenePtr += 8;
                break;
            case CS_CMD_TEXTBOX:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cmd = (CsCmdBase*)cutscenePtr;
                    if (cmd->base != 0xFFFF) {
                        Cutscene_Command_Textbox(play, csCtx, (void*)cutscenePtr);
                    }
                    cutscenePtr += 0xC;
                }
                break;
            case CS_CMD_SCENE_TRANS_FX:
                cutscenePtr += 4;
                Cutscene_Command_TransitionFX(play, csCtx, (void*)cutscenePtr);
                cutscenePtr += 8;
                break;
                
            // z64rom
            case CS_CMD_EXITPARAM:
                cutscenePtr = CutsceneCmd_ExitParam(play, cutscenePtr);
                break;
                
            default:
                MemCpy(&cmdEntries, cutscenePtr, 4);
                cutscenePtr += 4;
                for (j = 0; j < cmdEntries; j++) {
                    cutscenePtr += 0x30;
                }
                break;
        }
    }
}

void Cutscene_PlaySegment(PlayState* play, void* segment) {
    Cutscene_SetSegment(play, segment);
    gSaveContext.cutsceneTrigger = 1;
}

#include "SpawnCutsceneTable"

Asm_VanillaHook(Cutscene_HandleEntranceTriggers);
void Cutscene_HandleEntranceTriggers(PlayState* play) {
    u8 spawnID;
    u8 sceneID;
    s32 playSegment = false;
    s32 isSegment = false;
    void* segment = NULL;
    
    if (gExitParam.isExit) {
        sceneID = gExitParam.exit.sceneIndex;
        spawnID = gExitParam.exit.spawnIndex;
    } else {
        u16 entrance;
        
        if (!IS_DAY) {
            if (!LINK_IS_ADULT) entrance = play->nextEntranceIndex + 1;
            else entrance = play->nextEntranceIndex + 3;
        } else {
            if (!LINK_IS_ADULT) entrance = play->nextEntranceIndex;
            else entrance = play->nextEntranceIndex + 2;
        }
        
        sceneID = gEntranceTable[entrance].sceneId;
        spawnID = gEntranceTable[entrance].spawn;
    }
    
    for (s32 i = 0; i <= ARRAY_COUNT(sSpawnCutsceneTable); i++) {
        if (playSegment) {
            if (isSegment)
                segment = sSpawnCutsceneTable[i].segment;
            
            Assert(segment != NULL);
            Cutscene_SetSegment(play, segment);
            gSaveContext.cutsceneTrigger = 2;
            gSaveContext.showTitleCard = false;
            break;
        }
        
        if (i == ARRAY_COUNT(sSpawnCutsceneTable))
            break;
        
        // Enable hardcoded segments
        if (isSegment) {
            isSegment = false;
            continue;
        }
        isSegment = sSpawnCutsceneTable[i].nextIsSegment;
        
        if (sceneID != sSpawnCutsceneTable[i].scene)
            continue;
        if (spawnID != sSpawnCutsceneTable[i].spawn)
            continue;
        if (sSpawnCutsceneTable[i].age != 2 && sSpawnCutsceneTable[i].age != gSaveContext.linkAge)
            continue;
        
        switch (sSpawnCutsceneTable[i].type) {
            case FLAG_SWITCH:
                if (Flags_GetSwitch(play, sSpawnCutsceneTable[i].flag))
                    return;
                Flags_SetSwitch(play, sSpawnCutsceneTable[i].flag);
                
                break;
            case FLAG_CHEST:
                if (Flags_GetTreasure(play, sSpawnCutsceneTable[i].flag))
                    return;
                Flags_SetTreasure(play, sSpawnCutsceneTable[i].flag);
                
                break;
            case FLAG_EVENTCHKINF:
                if (GET_EVENTCHKINF(sSpawnCutsceneTable[i].flag))
                    return;
                SET_EVENTCHKINF(sSpawnCutsceneTable[i].flag);
                
                break;
            case FLAG_COLLECTIBLE:
                if (Flags_GetCollectible(play, sSpawnCutsceneTable[i].flag))
                    return;
                Flags_SetCollectible(play, sSpawnCutsceneTable[i].flag);
                
                break;
        }
        
        playSegment = true;
        if (!isSegment)
            segment = Segment_Scene_GetCutscene(play->sceneSegment, sSpawnCutsceneTable[i].header);
    }
}
