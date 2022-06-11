#include <uLib.h>

/*
   z64ram = 0x8010B0C0
   z64rom = 0xB82260
   z64next = 0x8010B680
 */

typedef enum {
	/*  0 */ TEXTBOX_TYPE_BLACK,
	/*  1 */ TEXTBOX_TYPE_WOODEN,
	/*  2 */ TEXTBOX_TYPE_BLUE,
	/*  3 */ TEXTBOX_TYPE_OCARINA,
	/*  4 */ TEXTBOX_TYPE_NONE_BOTTOM,
	/*  5 */ TEXTBOX_TYPE_NONE_NO_SHADOW,
	/* 11 */ TEXTBOX_TYPE_CREDITS = 11
} TextBoxType;

#define GET_EVENTCHKINF(flag) (gSaveContext.eventChkInf[(flag) >> 4] & (1 << ((flag) & 0xF)))

void Message_OpenText(GlobalContext* globalCtx, u16 textId) {
	static s16 messageStaticIndices[] = { 0, 1, 3, 2 };
	MessageContext* msgCtx = &globalCtx->msgCtx;
	Font* font = &msgCtx->font;
	s16 textBoxType;
	
	if (msgCtx->msgMode == MSGMODE_NONE) {
		gSaveContext.unk_13EE = gSaveContext.unk_13EA;
	}
	if (YREG(15) == 0x10) {
		Interface_ChangeAlpha(5);
	}
	
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
	if (textId == 0xC2 || textId == 0xFA) {
		// Increments text id based on piece of heart count, assumes the piece of heart text is all
		// in order and that you don't have more than the intended amount of heart pieces.
		textId += (gSaveContext.inventory.questItems & 0xF0000000 & 0xF0000000) >> 28;
	} else if (msgCtx->textId == 0xC && CHECK_OWNED_EQUIP(0, 2)) {
		textId = 0xB; // Traded Giant's Knife for Biggoron Sword
	} else if (msgCtx->textId == 0xB4 && GET_EVENTCHKINF(0x96)) {
		textId = 0xB5; // Destroyed Gold Skulltula
	}
	// Ocarina Staff + Dialog
	if (textId == 0x4077 || // Pierre?
		textId == 0x407A || // Pierre?
		textId == 0x2061 || // Learning Epona's Song
		textId == 0x5035 || // Guru-Guru in Windmill
		textId == 0x40AC) { // Ocarina Frog Minigame
		Interface_ChangeAlpha(1);
	}
	msgCtx->textId = textId;
	
	if (textId == 0x2030)  // Talking to Ingo as adult in Lon Lon Ranch for the first time before freeing Epona
		gSaveContext.eventInf[0] = gSaveContext.eventInf[1] = gSaveContext.eventInf[2] = gSaveContext.eventInf[3] = 0;
	
	if (sTextIsCredits) {
		Message_FindCreditsMessage(globalCtx, textId);
		msgCtx->msgLength = font->msgLength;
		DmaMgr_SendRequest1(
			font->msgBuf,
			gDmaDataTable[24].vromStart + font->msgOffset,
			font->msgLength,
			"",
			0
		);
	} else {
		Message_FindMessage(globalCtx, textId);
		msgCtx->msgLength = font->msgLength;
		DmaMgr_SendRequest1(
			font->msgBuf,
			gDmaDataTable[21].vromStart + font->msgOffset,
			font->msgLength,
			"",
			0
		);
	}
	msgCtx->textBoxProperties = font->charTexBuf[0];
	msgCtx->textBoxType = msgCtx->textBoxProperties >> 4;
	msgCtx->textBoxPos = msgCtx->textBoxProperties & 0xF;
	textBoxType = msgCtx->textBoxType;
	
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