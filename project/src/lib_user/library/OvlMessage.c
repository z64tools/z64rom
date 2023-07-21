#include <uLib.h>
#include <code/z_message_PAL.h>
#include <message_data_fmt.h>

typedef struct {
	OvlMessage* msg;
	OvlMessage* next;
	Color_RGB8  colorReg[0xF - 0x7];
	s8 curBox;
} OvlMsgState;

static OvlMsgState __msg;
static OvlMsgState* this = &__msg;

static s16 MsgLen(const char* msg) {
	s16 sz = 0;
	
	while (msg[sz] != '\0') {
		switch (msg[sz]) {
			case '\x11':
			case '\x12':
				sz++;
				
			case '\x05':
			case '\x06':
			case '\x0C':
			case '\x0E':
			case '\x13':
			case '\x14':
			case '\x1E':
				sz++;
				
			default:
				break;
		}
		
		sz++;
	}
	
	return sz;
}

static s16 OvlMessage_ConvertTo(char* dst) {
	const OvlMessage* msg = this->msg;
	s16 len = MsgLen(msg->txt);
	
	for (s32 i = 0; i <= len; i++) {
		switch (msg->txt[i]) {
			case '\0':
				dst[i] = CTRL_END;
				break;
				
			case '\n':
				dst[i] = CTRL_NEWLINE;
				break;
				
			case '\r':
				dst[i] = CTRL_BOX_BREAK;
				break;
				
			case '\2':
				dst[i] = 0x1B;
				break;
				
			case '\3':
				dst[i] = 0x1C;
				break;
				
			case '\xFA':
				dst[i] = '\x0A';
				break;
				
			// Skip 2 bytes
			case '\x11':
			case '\x12':
				dst[i] = msg->txt[i]; i++;
				
			// Skip 1 byte
			case '\x05':
			case '\x06':
			case '\x0C':
			case '\x0E':
			case '\x13':
			case '\x14':
			case '\x1E':
				dst[i] = msg->txt[i]; i++;
			default:
				dst[i] = msg->txt[i];
		}
	}
	
	if (this->next) {
		dst[len++] = CTRL_TEXTID;
		dst[len++] = 0xDE;
		dst[len++] = 0x01;
		dst[len] = CTRL_END;
	}
	
	return len;
}

/**
 * Initializes Actor with required flags
 */
void OvlMessage_Init(Actor* actor, OvlMsgType type) {
	switch (type) {
		case OVL_MSG_CHECK:
			actor->flags |= (ACTOR_FLAG_3 | ACTOR_FLAG_18);
			break;
			
		case OVL_MSG_TALK:
			actor->flags |= (ACTOR_FLAG_3 | ACTOR_FLAG_0);
			break;
	}
}

/**
 * Adds a color to certain register, for example: MSG_COLOR_REG_0
 */
void OvlMessage_RegisterColor(u8 id, u8 r, u8 g, u8 b) {
	Assert(id < 8);
	
	this->colorReg[id].r = r;
	this->colorReg[id].g = g;
	this->colorReg[id].b = b;
}

/**
 * Enables prompt with **radius** sphere. If talk process is initialized,
 * returns 1.
 *
 * If the conditions in order to talk with the actor are ok,
 * returns -1.
 */
s8 OvlMessage_Prompt(Actor* actor, OvlMessage* msg, f32 radius, u32 exchangeItemId) {
	PlayState* play = Effect_GetPlayState();
	
	actor->textId = 0xDE00;
	
	if (Actor_ProcessTalkRequest(actor, play))
		return 1;
	
	if (!func_8002F1C4(actor, play, radius, radius, exchangeItemId))
		return -1;
	
	this->msg = msg;
	
	return 0;
}

/**
 * Start conversation.
 */
void OvlMessage_Start(Actor* actor, OvlMessage* msg) {
	PlayState* play = Effect_GetPlayState();
	
	this->msg = msg;
	Message_StartTextbox(play, 0xDE00, actor);
}

/**
 * Forces conversation to continue. Main usage: after choice.
 */
void OvlMessage_Continue(Actor* actor, OvlMessage* msg) {
	PlayState* play = Effect_GetPlayState();
	
	this->msg = msg;
	Message_ContinueTextbox(play, 0xDE00);
	play->msgCtx.talkActor = actor;
}

/**
 * Moves the camera to focus this actor.
 */
s8 OvlMessage_SwapFocus(Actor* actor) {
	PlayState* play = Effect_GetPlayState();
	Player* p = GET_PLAYER(play);
	Camera* cam = GET_ACTIVE_CAM(play);
	
	if (cam->mode == CAM_MODE_TALK && cam->target != actor) {
		p->targetActor = actor; // Talk Actor
		p->unk_664 = actor; // Target Actor
		cam->target = actor;
		
		cam->animState = 0;
		R_RELOAD_CAM_PARAMS = true;
		return true;
	}
	
	return false;
}

s8 OvlMessage_IsClosed(Actor* actor) {
	PlayState* play = Effect_GetPlayState();
	
	if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
		return true;
	}
	
	return false;
}

s8 OvlMessage_IsCurrentMessage(OvlMessage* msg) {
	return msg == this->msg;
}

/**
 * Prepares a branch for current message.
 */
s8 OvlMessage_SetBranch(OvlMessage* branch) {
	MessageContext* msgCtx = &Effect_GetPlayState()->msgCtx;
	char* msg = msgCtx->font.msgBuf;
	s8 textEdit = false;
	
	if (msg[msgCtx->msgLength - 3] != CTRL_TEXTID) {
		osLibPrintf("Patched Text");
		
		msg[msgCtx->msgLength++] = CTRL_TEXTID;
		msg[msgCtx->msgLength++] = 0xDE;
		msg[msgCtx->msgLength++] = 0x01;
		msg[msgCtx->msgLength] = CTRL_END;
		
		textEdit = true;
	}
	
	this->next = branch;
	
	return textEdit;
}

/**
 * Returns choice Index + 1. 0 return will mean that the choice hasn't
 * been made yet.
 */
s8 OvlMessage_GetChoice(Actor* actor) {
	PlayState* play = Effect_GetPlayState();
	
	if (Message_GetState(&play->msgCtx) == TEXT_STATE_CHOICE && Message_ShouldAdvanceSilent(play))
		return play->msgCtx.choiceIndex + 1;
	
	return 0;
}

/**
 * Current box being the box X from the current message. New messages will
 * reset this counter. MSG_EVENT is mostly used to do something on a certain
 * box but will be providing this anyway!
 */
s8 OvlMessage_GetBoxNum(void) {
	return this->curBox;
}

////////////////////////////////////////////////////////////////////////////////

void OvlMessage_Update(PlayState* play) {
	if (Message_GetState(&play->msgCtx))
		this->msg = NULL;
}

////////////////////////////////////////////////////////////////////////////////

Asm_VanillaHook(Message_OpenText);
void Message_OpenText(PlayState* playState, u16 textId) {
	const s16 messageStaticIndices[] = { 0, 1, 3, 2 };
	MessageContext* msgCtx = &playState->msgCtx;
	Font* font = &msgCtx->font;
	s16 textBoxType;
	
	this->curBox = 0;
	
	if (msgCtx->msgMode == MSGMODE_NONE)
		gSaveContext.unk_13EE = gSaveContext.unk_13EA;
	
	if (YREG(15) == 0x10)
		Interface_ChangeAlpha(5);
	
	sMessageHasSetSfx = D_8014B2F4 = sTextboxSkipped = sTextIsCredits = 0;
	
	if (textId >= 0x0500 && textId < 0x0600) { // text ids 0500 to 0600 are reserved for credits
		sTextIsCredits = true;
		R_TEXT_CHAR_SCALE = 85;
		R_TEXT_LINE_SPACING = 6;
		R_TEXT_INIT_XPOS = 20;
		YREG(1) = 48;
	} else {
		R_TEXT_CHAR_SCALE = 75;
		R_TEXT_LINE_SPACING = 12;
		R_TEXT_INIT_XPOS = 65;
	}
	
	if (textId == 0xC2 || textId == 0xFA)
		// Increments text id based on piece of heart count, assumes the piece of heart text is all
		// in order and that you don't have more than the intended amount of heart pieces.
		textId += (gSaveContext.inventory.questItems & 0xF0000000 & 0xF0000000) >> 28;
	
	else if (msgCtx->textId == 0xC && CHECK_OWNED_EQUIP(0, 2))
		textId = 0xB; // Traded Giant's Knife for Biggoron Sword
	
	else if (msgCtx->textId == 0xB4 && GET_EVENTCHKINF(0x96))
		textId = 0xB5; // Destroyed Gold Skulltula
	
	// Ocarina Staff + Dialog
	if (textId == 0x4077 || // Pierre?
		textId == 0x407A || // Pierre?
		textId == 0x2061 || // Learning Epona's Song
		textId == 0x5035 || // Guru-Guru in Windmill
		textId == 0x40AC) // Ocarina Frog Minigame
		Interface_ChangeAlpha(1);
	
	if (textId == 0xDE01) {
		this->msg = this->next;
		this->next = NULL;
		textId = 0xDE00;
	}
	
	msgCtx->textId = textId;
	
	if (textId == 0x2030)      // Talking to Ingo as adult in Lon Lon Ranch for the first time before freeing Epona
		gSaveContext.eventInf[0] = gSaveContext.eventInf[1] = gSaveContext.eventInf[2] = gSaveContext.eventInf[3] = 0;
	
	if (sTextIsCredits) {
		Message_FindCreditsMessage(playState, textId);
		msgCtx->msgLength = font->msgLength;
		DmaMgr_SendRequest1(font->msgBuf, gDmaDataTable[24].vromStart + font->msgOffset, font->msgLength, "", 0);
	} else if (textId != 0xDE00) {
		Message_FindMessage(playState, textId);
		msgCtx->msgLength = font->msgLength;
		DmaMgr_SendRequest1(font->msgBuf, gDmaDataTable[21].vromStart + font->msgOffset, font->msgLength, "", 0);
	}
	
	msgCtx->textBoxProperties = font->charTexBuf[0];
	msgCtx->textBoxType = msgCtx->textBoxProperties >> 4;
	msgCtx->textBoxPos = msgCtx->textBoxProperties & 0xF;
	textBoxType = msgCtx->textBoxType;
	
	if (textId == 0xDE00) {
		osLibPrintf(osInfo(""));
		osLibPrintf("LoadMsg");
		msgCtx->msgLength = OvlMessage_ConvertTo(font->msgBuf);
		msgCtx->textBoxType = textBoxType = this->msg->type;
		msgCtx->textBoxPos = this->msg->pos;
	}
	
	if (textBoxType < TEXTBOX_TYPE_NONE_BOTTOM) {
		DmaMgr_SendRequest1(
			msgCtx->textboxSegment,
			gDmaDataTable[18].vromStart + (messageStaticIndices[textBoxType] * MESSAGE_STATIC_TEX_SIZE),
			MESSAGE_STATIC_TEX_SIZE,
			"",
			0
		);
		if (textBoxType == TEXTBOX_TYPE_BLACK) {
			msgCtx->textboxColorRed = 0;
			msgCtx->textboxColorGreen = 0;
			msgCtx->textboxColorBlue = 0;
		} else if (textBoxType == TEXTBOX_TYPE_WOODEN) {
			msgCtx->textboxColorRed = 70;
			msgCtx->textboxColorGreen = 50;
			msgCtx->textboxColorBlue = 30;
		} else if (textBoxType == TEXTBOX_TYPE_BLUE) {
			msgCtx->textboxColorRed = 0;
			msgCtx->textboxColorGreen = 10;
			msgCtx->textboxColorBlue = 50;
		} else {
			msgCtx->textboxColorRed = 255;
			msgCtx->textboxColorGreen = 0;
			msgCtx->textboxColorBlue = 0;
		}
		if (textBoxType == TEXTBOX_TYPE_WOODEN) {
			msgCtx->textboxColorAlphaTarget = 230;
		} else if (textBoxType == TEXTBOX_TYPE_OCARINA) {
			msgCtx->textboxColorAlphaTarget = 180;
		} else {
			msgCtx->textboxColorAlphaTarget = 170;
		}
		msgCtx->textboxColorAlphaCurrent = 0;
	}
	msgCtx->choiceNum = msgCtx->textUnskippable = msgCtx->textboxEndType = 0;
	msgCtx->msgBufPos = msgCtx->unk_E3D0 = msgCtx->textDrawPos = 0;
}

Asm_VanillaHook(Message_SetTextColor);
void Message_SetTextColor(MessageContext* msgCtx, u16 colorParameter) {
	switch (colorParameter) {
		case MSGCOL_RED:
			if (msgCtx->textBoxType == TEXTBOX_TYPE_WOODEN) {
				msgCtx->textColorR = 255;
				msgCtx->textColorG = 120;
				msgCtx->textColorB = 0;
			} else {
				msgCtx->textColorR = 255;
				msgCtx->textColorG = 60;
				msgCtx->textColorB = 60;
			}
			break;
		case MSGCOL_ADJUSTABLE: // GREEN
			if (msgCtx->textBoxType == TEXTBOX_TYPE_WOODEN) {
				msgCtx->textColorR = R_TEXT_ADJUST_COLOR_1_R;
				msgCtx->textColorG = R_TEXT_ADJUST_COLOR_1_G;
				msgCtx->textColorB = R_TEXT_ADJUST_COLOR_1_B;
			} else {
				msgCtx->textColorR = R_TEXT_ADJUST_COLOR_2_R;
				msgCtx->textColorG = R_TEXT_ADJUST_COLOR_2_G;
				msgCtx->textColorB = R_TEXT_ADJUST_COLOR_2_B;
			}
			break;
		case MSGCOL_BLUE:
			if (msgCtx->textBoxType == TEXTBOX_TYPE_WOODEN) {
				msgCtx->textColorR = 80;
				msgCtx->textColorG = 110;
				msgCtx->textColorB = 255;
			} else {
				msgCtx->textColorR = 80;
				msgCtx->textColorG = 90;
				msgCtx->textColorB = 255;
			}
			break;
		case MSGCOL_LIGHTBLUE:
			if (msgCtx->textBoxType == TEXTBOX_TYPE_WOODEN) {
				msgCtx->textColorR = 90;
				msgCtx->textColorG = 180;
				msgCtx->textColorB = 255;
			} else if (msgCtx->textBoxType == TEXTBOX_TYPE_NONE_NO_SHADOW) {
				msgCtx->textColorR = 80;
				msgCtx->textColorG = 150;
				msgCtx->textColorB = 180;
			} else {
				msgCtx->textColorR = 100;
				msgCtx->textColorG = 180;
				msgCtx->textColorB = 255;
			}
			break;
		case MSGCOL_PURPLE:
			if (msgCtx->textBoxType == TEXTBOX_TYPE_WOODEN) {
				msgCtx->textColorR = 210;
				msgCtx->textColorG = 100;
				msgCtx->textColorB = 255;
			} else {
				msgCtx->textColorR = 255;
				msgCtx->textColorG = 150;
				msgCtx->textColorB = 180;
			}
			break;
		case MSGCOL_YELLOW:
			if (msgCtx->textBoxType == TEXTBOX_TYPE_WOODEN) {
				msgCtx->textColorR = 255;
				msgCtx->textColorG = 255;
				msgCtx->textColorB = 30;
			} else {
				msgCtx->textColorR = 225;
				msgCtx->textColorG = 255;
				msgCtx->textColorB = 50;
			}
			break;
		case MSGCOL_BLACK:
			msgCtx->textColorR = msgCtx->textColorG = msgCtx->textColorB = 0;
			break;
			
		case 0x08 ... 0x0F:
			(void)0;
			u32 colRegId = colorParameter - 0x8;
			
			msgCtx->textColorR = this->colorReg[colRegId].r;
			msgCtx->textColorG = this->colorReg[colRegId].g;
			msgCtx->textColorB = this->colorReg[colRegId].b;
			
			break;
			
		case MSGCOL_DEFAULT:
		default:
			if (msgCtx->textBoxType == TEXTBOX_TYPE_NONE_NO_SHADOW) {
				msgCtx->textColorR = msgCtx->textColorG = msgCtx->textColorB = 0;
			} else {
				msgCtx->textColorR = msgCtx->textColorG = msgCtx->textColorB = 255;
			}
			break;
	}
}

Asm_VanillaHook(Message_ShouldAdvance);
u8 Message_ShouldAdvance(PlayState* play) {
	Input* input = &play->state.input[0];
	
	if (CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_B) ||
		CHECK_BTN_ALL(input->press.button, BTN_CUP)) {
		Audio_PlaySfxGeneral(NA_SE_SY_MESSAGE_PASS, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
			&gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
		sMessageHasSetSfx = false;
		this->curBox++;
	}
	return CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_B) ||
		   CHECK_BTN_ALL(input->press.button, BTN_CUP);
}
