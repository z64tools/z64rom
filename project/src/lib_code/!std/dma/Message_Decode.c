#include <uLib.h>
#include <message_data_fmt.h>
#include <message_data_static.h>
#include "code/z_message_PAL.h"

/*
   z64ram = 0x80109B3C
   z64rom = 0xB80CDC
 */

void Message_Decode(PlayState* playState) {
    u8 temp_s2;
    u8 phi_s1;
    u16 phi_s0_3;
    s32 loadChar;
    s32 charTexIdx = 0;
    s16 playerNameLen;
    s16 decodedBufPos = 0;
    s16 numLines = 0;
    s16 i;
    s16 digits[4];
    f32 timeInSeconds;
    MessageContext* msgCtx = &playState->msgCtx;
    Font* font = &playState->msgCtx.font;
    
    playState->msgCtx.textDelayTimer = 0;
    playState->msgCtx.textUnskippable = playState->msgCtx.textDelay = playState->msgCtx.textDelayTimer = 0;
    sTextFade = false;
    
    while (true) {
        phi_s1 = temp_s2 = msgCtx->msgBufDecoded[decodedBufPos] = font->msgBuf[msgCtx->msgBufPos];
        
        if (
            temp_s2 == MESSAGE_BOX_BREAK || temp_s2 == MESSAGE_TEXTID || temp_s2 == MESSAGE_BOX_BREAK_DELAYED ||
            temp_s2 == MESSAGE_EVENT || temp_s2 == MESSAGE_END
        ) {
            // Textbox decoding ends with any of the above text control characters
            msgCtx->msgMode = MSGMODE_TEXT_DISPLAYING;
            msgCtx->textDrawPos = 1;
            R_TEXT_INIT_YPOS = R_TEXTBOX_Y + 8;
            if (msgCtx->textBoxType != TEXTBOX_TYPE_NONE_BOTTOM) {
                if (numLines == 0) {
                    R_TEXT_INIT_YPOS = (u16)(R_TEXTBOX_Y + 26);
                } else if (numLines == 1) {
                    R_TEXT_INIT_YPOS = (u16)(R_TEXTBOX_Y + 20);
                } else if (numLines == 2) {
                    R_TEXT_INIT_YPOS = (u16)(R_TEXTBOX_Y + 16);
                }
            }
            if (phi_s1 == MESSAGE_TEXTID) {
                temp_s2 = msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[msgCtx->msgBufPos + 1];
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[msgCtx->msgBufPos + 2];
                phi_s0_3 = temp_s2 << 8;
                sNextTextId = msgCtx->msgBufDecoded[decodedBufPos] | phi_s0_3;
            }
            if (phi_s1 == MESSAGE_BOX_BREAK_DELAYED) {
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[msgCtx->msgBufPos + 1];
                msgCtx->msgBufPos += 2;
            }
            msgCtx->decodedTextLen = decodedBufPos;
            if (sTextboxSkipped) {
                msgCtx->textDrawPos = msgCtx->decodedTextLen;
            }
            break;
        } else if (temp_s2 == MESSAGE_NAME) {
            // Substitute the player name control character for the file's player name.
            for (playerNameLen = ARRAY_COUNT(gSaveContext.playerName); playerNameLen > 0; playerNameLen--) {
                if (gSaveContext.playerName[playerNameLen - 1] != 0x3E) {
                    break;
                }
            }
            
            for (i = 0; i < playerNameLen; i++) {
                phi_s1 = gSaveContext.playerName[i];
                if (phi_s1 == 0x3E) {
                    phi_s1 = ' ';
                } else if (phi_s1 == 0x40) {
                    phi_s1 = '.';
                } else if (phi_s1 == 0x3F) {
                    phi_s1 = '-';
                } else if (phi_s1 < 0xA) {
                    phi_s1 += 0;
                    phi_s1 += '0';
                } else if (phi_s1 < 0x24) {
                    phi_s1 += 0;
                    phi_s1 += '7';
                } else if (phi_s1 < 0x3E) {
                    phi_s1 += 0;
                    phi_s1 += '=';
                }
                if (phi_s1 != ' ') {
                    Font_LoadChar(font, phi_s1 - ' ', charTexIdx);
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                }
                
                msgCtx->msgBufDecoded[decodedBufPos] = phi_s1;
                decodedBufPos++;
            }
            decodedBufPos--;
        } else if (temp_s2 == MESSAGE_MARATHON_TIME || temp_s2 == MESSAGE_RACE_TIME) {
            // Convert the values of the appropriate timer to digits and add the
            //  digits to the decoded buffer in place of the control character.
            // "EVENT timer"
            
            digits[0] = digits[1] = digits[2] = 0;
            if (temp_s2 == MESSAGE_RACE_TIME) {
                digits[3] = gSaveContext.timer1Value;
            } else {
                digits[3] = gSaveContext.timer2Value;
            }
            
            while (digits[3] >= 60) {
                digits[1]++;
                if (digits[1] >= 10) {
                    digits[0]++;
                    digits[1] -= 10;
                }
                digits[3] -= 60;
            }
            while (digits[3] >= 10) {
                digits[2]++;
                digits[3] -= 10;
            }
            
            for (i = 0; i < 4; i++) {
                Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                charTexIdx += FONT_CHAR_TEX_SIZE;
                msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                decodedBufPos++;
                if (i == 1) {
                    Font_LoadChar(font, '"' - ' ', charTexIdx);
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                    msgCtx->msgBufDecoded[decodedBufPos] = '"';
                    decodedBufPos++;
                } else if (i == 3) {
                    Font_LoadChar(font, '"' - ' ', charTexIdx);
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                    msgCtx->msgBufDecoded[decodedBufPos] = '"';
                }
            }
        } else if (temp_s2 == MESSAGE_POINTS) {
            // Convert the values of the current minigame score to digits and
            //  add the digits to the decoded buffer in place of the control character.
            // "Yabusame score"
            
            digits[0] = digits[1] = digits[2] = 0;
            digits[3] = gSaveContext.minigameScore;
            
            while (digits[3] >= 1000) {
                digits[0]++;
                digits[3] -= 1000;
            }
            while (digits[3] >= 100) {
                digits[1]++;
                digits[3] -= 100;
            }
            while (digits[3] >= 10) {
                digits[2]++;
                digits[3] -= 10;
            }
            
            loadChar = false;
            for (i = 0; i < 4; i++) {
                if (i == 3 || digits[i] != 0) {
                    loadChar = true;
                }
                if (loadChar) {
                    Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                    msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                    decodedBufPos++;
                }
            }
            decodedBufPos--;
        } else if (temp_s2 == MESSAGE_TOKENS) {
            // Convert the current number of collected gold skulltula tokens to digits and
            //  add the digits to the decoded buffer in place of the control character.
            // "Total number of gold stars"
            
            digits[0] = digits[1] = 0;
            digits[2] = gSaveContext.inventory.gsTokens;
            
            while (digits[2] >= 100) {
                digits[0]++;
                digits[2] -= 100;
            }
            while (digits[2] >= 10) {
                digits[1]++;
                digits[2] -= 10;
            }
            
            loadChar = false;
            for (i = 0; i < 3; i++) {
                if (i == 2 || digits[i] != 0) {
                    loadChar = true;
                }
                if (loadChar) {
                    Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                    msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                    
                    decodedBufPos++;
                }
            }
            decodedBufPos--;
        } else if (temp_s2 == MESSAGE_FISH_INFO) {
            // "Fishing hole fish size"
            
            digits[0] = 0;
            digits[1] = gSaveContext.minigameScore;
            
            while (digits[1] >= 10) {
                digits[0]++;
                digits[1] -= 10;
            }
            
            for (i = 0; i < 2; i++) {
                if (i == 1 || digits[i] != 0) {
                    Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                    msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                    
                    decodedBufPos++;
                }
            }
            decodedBufPos--;
        } else if (temp_s2 == MESSAGE_HIGHSCORE) {
            phi_s0_3 = HIGH_SCORE((u8)font->msgBuf[++msgCtx->msgBufPos]);
            // "Highscore"
            
            if ((font->msgBuf[msgCtx->msgBufPos] & 0xFF) == 2) {
                if (LINK_AGE_IN_YEARS == YEARS_CHILD) {
                    phi_s0_3 &= 0x7F;
                } else {
                    phi_s0_3 = ((HIGH_SCORE((u8)font->msgBuf[msgCtx->msgBufPos]) & 0xFF000000) >> 0x18) & 0x7F;
                }
                phi_s0_3 = SQ((f32)phi_s0_3) * 0.0036f + 0.5f;
            }
            switch (font->msgBuf[msgCtx->msgBufPos] & 0xFF) {
                case HS_HBA:
                case HS_POE_POINTS:
                case HS_FISHING:
                    digits[0] = digits[1] = digits[2] = 0;
                    digits[3] = phi_s0_3;
                    
                    while (digits[3] >= 1000) {
                        digits[0]++;
                        digits[3] -= 1000;
                    }
                    while (digits[3] >= 100) {
                        digits[1]++;
                        digits[3] -= 100;
                    }
                    while (digits[3] >= 10) {
                        digits[2]++;
                        digits[3] -= 10;
                    }
                    if (temp_s2) {
                    }
                    
                    loadChar = false;
                    for (i = 0; i < 4; i++) {
                        if (i == 3 || digits[i] != 0) {
                            loadChar = true;
                        }
                        if (loadChar) {
                            Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                            msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                            charTexIdx += FONT_CHAR_TEX_SIZE;
                            decodedBufPos++;
                        }
                    }
                    decodedBufPos--;
                    break;
                case HS_UNK_05:
                    break;
                case HS_HORSE_RACE:
                case HS_MARATHON:
                case HS_DAMPE_RACE:
                    digits[0] = digits[1] = digits[2] = 0;
                    digits[3] = phi_s0_3;
                    
                    while (digits[3] >= 60) {
                        digits[1]++;
                        if (digits[1] >= 10) {
                            digits[0]++;
                            digits[1] -= 10;
                        }
                        digits[3] -= 60;
                    }
                    while (digits[3] >= 10) {
                        digits[2]++;
                        digits[3] -= 10;
                    }
                    
                    for (i = 0; i < 4; i++) {
                        Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                        charTexIdx += FONT_CHAR_TEX_SIZE;
                        msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                        decodedBufPos++;
                        if (i == 1) {
                            Font_LoadChar(font, '"' - ' ', charTexIdx);
                            charTexIdx += FONT_CHAR_TEX_SIZE;
                            msgCtx->msgBufDecoded[decodedBufPos] = '"';
                            decodedBufPos++;
                        } else if (i == 3) {
                            Font_LoadChar(font, '"' - ' ', charTexIdx);
                            charTexIdx += FONT_CHAR_TEX_SIZE;
                            msgCtx->msgBufDecoded[decodedBufPos] = '"';
                        }
                    }
                    break;
            }
        } else if (temp_s2 == MESSAGE_TIME) {
            // "Zelda time"
            digits[0] = 0;
            timeInSeconds = gSaveContext.dayTime * (24.0f * 60.0f / 0x10000);
            
            digits[1] = timeInSeconds / 60.0f;
            while (digits[1] >= 10) {
                digits[0]++;
                digits[1] -= 10;
            }
            digits[2] = 0;
            digits[3] = (s16)timeInSeconds % 60;
            while (digits[3] >= 10) {
                digits[2]++;
                digits[3] -= 10;
            }
            
            for (i = 0; i < 4; i++) {
                Font_LoadChar(font, digits[i] + '0' - ' ', charTexIdx);
                charTexIdx += FONT_CHAR_TEX_SIZE;
                msgCtx->msgBufDecoded[decodedBufPos] = digits[i] + '0';
                decodedBufPos++;
                if (i == 1) {
                    Font_LoadChar(font, ':' - ' ', charTexIdx);
                    charTexIdx += FONT_CHAR_TEX_SIZE;
                    msgCtx->msgBufDecoded[decodedBufPos] = ':';
                    decodedBufPos++;
                }
            }
            decodedBufPos--;
        } else if (temp_s2 == MESSAGE_ITEM_ICON) {
            msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[msgCtx->msgBufPos + 1];
            
            Message_LoadItemIcon(playState, font->msgBuf[msgCtx->msgBufPos + 1], R_TEXTBOX_Y + 10);
        } else if (temp_s2 == MESSAGE_BACKGROUND) {
            msgCtx->textboxBackgroundIdx = font->msgBuf[msgCtx->msgBufPos + 1] * 2;
            msgCtx->textboxBackgroundForeColorIdx = (font->msgBuf[msgCtx->msgBufPos + 2] & 0xF0) >> 4;
            msgCtx->textboxBackgroundBackColorIdx = font->msgBuf[msgCtx->msgBufPos + 2] & 0xF;
            msgCtx->textboxBackgroundYOffsetIdx = (font->msgBuf[msgCtx->msgBufPos + 3] & 0xF0) >> 4;
            msgCtx->textboxBackgroundUnkArg = font->msgBuf[msgCtx->msgBufPos + 3] & 0xF;
            DmaMgr_SendRequest1(
                (void*)((u32)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE),
                (u32)gDmaDataTable[19].vromStart + msgCtx->textboxBackgroundIdx * 0x900,
                0x900,
                "Message_Decode",
                1830
            );
            DmaMgr_SendRequest1(
                (void*)((u32)msgCtx->textboxSegment + MESSAGE_STATIC_TEX_SIZE + 0x900),
                (u32)gDmaDataTable[19].vromStart +
                (msgCtx->textboxBackgroundIdx + 1) * 0x900,
                0x900,
                "Message_Decode",
                1834
            );
            msgCtx->msgBufPos += 3;
            R_TEXTBOX_BG_YPOS = R_TEXTBOX_Y + 8;
            numLines = 2;
            R_TEXT_INIT_XPOS = 50;
        } else if (temp_s2 == MESSAGE_COLOR) {
            msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos];
        } else if (temp_s2 == MESSAGE_NEWLINE) {
            numLines++;
        } else if (
            temp_s2 != MESSAGE_QUICKTEXT_ENABLE && temp_s2 != MESSAGE_QUICKTEXT_DISABLE &&
            temp_s2 != MESSAGE_AWAIT_BUTTON_PRESS && temp_s2 != MESSAGE_OCARINA &&
            temp_s2 != MESSAGE_PERSISTENT && temp_s2 != MESSAGE_UNSKIPPABLE
        ) {
            if (temp_s2 == MESSAGE_FADE) {
                sTextFade = true;
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos];
            } else if (temp_s2 == MESSAGE_FADE2) {
                sTextFade = true;
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos];
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos];
            } else if (temp_s2 == MESSAGE_SHIFT || temp_s2 == MESSAGE_TEXT_SPEED) {
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos] & 0xFF;
            } else if (temp_s2 == MESSAGE_SFX) {
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos];
                msgCtx->msgBufDecoded[++decodedBufPos] = font->msgBuf[++msgCtx->msgBufPos];
            } else if (temp_s2 == MESSAGE_TWO_CHOICE) {
                msgCtx->choiceNum = 2;
            } else if (temp_s2 == MESSAGE_THREE_CHOICE) {
                msgCtx->choiceNum = 3;
            } else if (temp_s2 != ' ') {
                Font_LoadChar(font, temp_s2 - ' ', charTexIdx);
                charTexIdx += FONT_CHAR_TEX_SIZE;
            }
        }
        decodedBufPos++;
        msgCtx->msgBufPos++;
    }
}
