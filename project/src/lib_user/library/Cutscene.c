#include <uLib.h>

#define TYPE_EVENT 0
#define TYPE_ITEM  1

typedef struct {
	struct {
		u8 __pad;      // OOOO-OOOO ****-****
		u8 exit : 1;   // ****-**** O***-****
		u8 set  : 1;   // ****-**** *O**-****
		u8 type : 6;   // ****-**** **OO-OOOO
	};
	u16 flag;
} CmdFlag;

typedef struct {
	u32 cmdId;
	NewExit exitParam;
	CmdFlag list[];
} CmdHeader;

void* CutsceneCmd_ExitParam(PlayState* play, void* ptr) {
	CmdHeader* cmd = ptr;
	
	play->transitionTrigger = TRANS_TRIGGER_START;
	gExitParam.nextEntranceIndex = cmd->exitParam.upper;
	gExitParam.exit = cmd->exitParam;
	
	for (s32 i = 0; cmd->list[i].exit == false; i++) {
		u16 type = cmd->list[i].type;
		u16 flag = cmd->list[i].flag;
		s8 set = cmd->list[i].set;
		
		switch (type) {
			case TYPE_EVENT:
				if (set) SET_EVENTCHKINF(flag);
				else CLEAR_EVENTCHKINF(flag);
				break;
				
			case TYPE_ITEM:
				if (set) Item_Give(play, flag);
				else Inventory_DeleteItem(flag, SLOT(flag));
				break;
		}
	}
	
	return cmd + 1;
}

#include <code/z_demo.h>

void Cutscene_ProcessCmds(PlayState* play,CutsceneContext* csCtx,u8* cutscenePtr) {
	s16 i;
	s32 totalEntries;
	s32 cmdType;
	s32 cmdEntries;
	CsCmdBase* cmd;
	s32 cutsceneEndFrame;
	s16 j;
	
	MemCpy(&totalEntries,cutscenePtr,4);
	cutscenePtr += 4;
	MemCpy(&cutsceneEndFrame,cutscenePtr,4);
	cutscenePtr += 4;
	
	if ((cutsceneEndFrame < csCtx->frames) && (csCtx->state != CS_STATE_UNSKIPPABLE_EXEC)) {
		csCtx->state = CS_STATE_UNSKIPPABLE_INIT;
		
		return;
	}
	
#ifdef DEV_BUILD
	if (CHECK_BTN_ALL(play->state.input[0].press.button,BTN_DRIGHT)) {
		csCtx->state = CS_STATE_UNSKIPPABLE_INIT;
		
		return;
	}
#endif
	
	for (i = 0; i < totalEntries; i++) {
		MemCpy(&cmdType,cutscenePtr,4);
		cutscenePtr += 4;
		
		if (cmdType == -1) {
			return;
		}
		
		switch (cmdType) {
			case CS_CMD_MISC:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					func_80064824(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0x30;
				}
				break;
			case CS_CMD_SET_LIGHTING:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					Cutscene_Command_SetLighting(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0x30;
				}
				break;
			case CS_CMD_PLAYBGM:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					Cutscene_Command_PlayBGM(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0x30;
				}
				break;
			case CS_CMD_STOPBGM:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					Cutscene_Command_StopBGM(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0x30;
				}
				break;
			case CS_CMD_FADEBGM:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					Cutscene_Command_FadeBGM(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0x30;
				}
				break;
			case CS_CMD_09:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					Cutscene_Command_09(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0xC;
				}
				break;
			case CS_CMD_SETTIME:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					func_80065134(play,csCtx,(void*)cutscenePtr);
					cutscenePtr += 0xC;
				}
				break;
			case CS_CMD_SET_PLAYER_ACTION:
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				MemCpy(&cmdEntries,cutscenePtr,4);
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
				cutscenePtr += Cutscene_Command_CameraEyePoints(play,csCtx,(void*)cutscenePtr,0);
				break;
			case CS_CMD_CAM_EYE_REL_TO_PLAYER:
				cutscenePtr += Cutscene_Command_CameraEyePoints(play,csCtx,(void*)cutscenePtr,1);
				break;
			case CS_CMD_CAM_AT:
				cutscenePtr += Cutscene_Command_CameraLookAtPoints(play,csCtx,(void*)cutscenePtr,0);
				break;
			case CS_CMD_CAM_AT_REL_TO_PLAYER:
				cutscenePtr += Cutscene_Command_CameraLookAtPoints(play,csCtx,(void*)cutscenePtr,1);
				break;
			case CS_CMD_07:
				cutscenePtr += Cutscene_Command_07(play,csCtx,(void*)cutscenePtr,0);
				break;
			case CS_CMD_08:
				cutscenePtr += Cutscene_Command_08(play,csCtx,(void*)cutscenePtr,0);
				break;
			case CS_CMD_TERMINATOR:
				cutscenePtr += 4;
				Cutscene_Command_Terminator(play,csCtx,(void*)cutscenePtr);
				cutscenePtr += 8;
				break;
			case CS_CMD_TEXTBOX:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					cmd = (CsCmdBase*)cutscenePtr;
					if (cmd->base != 0xFFFF) {
						Cutscene_Command_Textbox(play,csCtx,(void*)cutscenePtr);
					}
					cutscenePtr += 0xC;
				}
				break;
			case CS_CMD_SCENE_TRANS_FX:
				cutscenePtr += 4;
				Cutscene_Command_TransitionFX(play,csCtx,(void*)cutscenePtr);
				cutscenePtr += 8;
				break;
				
			// z64rom
			case CS_CMD_EXITPARAM:
				cutscenePtr = CutsceneCmd_ExitParam(play,cutscenePtr);
				break;
				
			default:
				MemCpy(&cmdEntries,cutscenePtr,4);
				cutscenePtr += 4;
				for (j = 0; j < cmdEntries; j++) {
					cutscenePtr += 0x30;
				}
				break;
		}
	}
}