#ifndef _FILE_CHOOSE_H_
#define _FILE_CHOOSE_H_

#include <oot_mq_debug/z64hdr.h>

#define GET_NEWF(sramCtx, slotNum, index) (sramCtx->readBuff[gSramSlotOffsets[slotNum] + OFFSETOF(SaveContext, newf[index])])

#define SLOT_OCCUPIED(sramCtx, slotNum) \
	((GET_NEWF(sramCtx, slotNum, 0) == 'Z') || \
	(GET_NEWF(sramCtx, slotNum, 1) == 'E') || \
	(GET_NEWF(sramCtx, slotNum, 2) == 'L') || \
	(GET_NEWF(sramCtx, slotNum, 3) == 'D') || \
	(GET_NEWF(sramCtx, slotNum, 4) == 'A') || \
	(GET_NEWF(sramCtx, slotNum, 5) == 'Z'))

// Init mode: Initial setup as the file select is starting up, fades and slides in various menu elements
// Config mode: Handles the bulk of the file select, various configuration tasks like picking a file, copy/erase, and the options menu
// Select mode: Displays the selected file with various details about it, and allows the player to confirm and open it
typedef enum {
	/* 0 */ FS_MENU_MODE_INIT,
	/* 1 */ FS_MENU_MODE_CONFIG,
	/* 2 */ FS_MENU_MODE_SELECT
} MenuMode;

typedef enum {
	/* 00 */ CM_FADE_IN_START,
	/* 01 */ CM_FADE_IN_END,
	/* 02 */ CM_MAIN_MENU,
	/* 03 */ CM_SETUP_COPY_SOURCE,
	/* 04 */ CM_SELECT_COPY_SOURCE,
	/* 05 */ CM_SETUP_COPY_DEST_1,
	/* 06 */ CM_SETUP_COPY_DEST_2,
	/* 07 */ CM_SELECT_COPY_DEST,
	/* 08 */ CM_EXIT_TO_COPY_SOURCE_1,
	/* 09 */ CM_EXIT_TO_COPY_SOURCE_2,
	/* 10 */ CM_SETUP_COPY_CONFIRM_1,
	/* 11 */ CM_SETUP_COPY_CONFIRM_2,
	/* 12 */ CM_COPY_CONFIRM,
	/* 13 */ CM_RETURN_TO_COPY_DEST,
	/* 14 */ CM_COPY_ANIM_1,
	/* 15 */ CM_COPY_ANIM_2,
	/* 16 */ CM_COPY_ANIM_3,
	/* 17 */ CM_COPY_ANIM_4,
	/* 18 */ CM_COPY_ANIM_5,
	/* 19 */ CM_COPY_RETURN_MAIN,
	/* 20 */ CM_SETUP_ERASE_SELECT,
	/* 21 */ CM_ERASE_SELECT,
	/* 22 */ CM_SETUP_ERASE_CONFIRM_1,
	/* 23 */ CM_SETUP_ERASE_CONFIRM_2,
	/* 24 */ CM_ERASE_CONFIRM,
	/* 25 */ CM_EXIT_TO_ERASE_SELECT_1,
	/* 26 */ CM_EXIT_TO_ERASE_SELECT_2,
	/* 27 */ CM_ERASE_ANIM_1,
	/* 28 */ CM_ERASE_ANIM_2,
	/* 29 */ CM_ERASE_ANIM_3,
	/* 30 */ CM_EXIT_ERASE_TO_MAIN,
	/* 31 */ CM_UNUSED_31,
	/* 32 */ CM_ROTATE_TO_NAME_ENTRY,
	/* 33 */ CM_NAME_ENTRY,
	/* 34 */ CM_START_NAME_ENTRY,
	/* 35 */ CM_NAME_ENTRY_TO_MAIN,
	/* 36 */ CM_MAIN_TO_OPTIONS,
	/* 37 */ CM_OPTIONS_MENU,
	/* 38 */ CM_START_OPTIONS,
	/* 39 */ CM_OPTIONS_TO_MAIN,
	/* 40 */ CM_UNUSED_DELAY
} ConfigMode;

typedef enum {
	/* 0 */ SM_FADE_MAIN_TO_SELECT,
	/* 1 */ SM_MOVE_FILE_TO_TOP,
	/* 2 */ SM_FADE_IN_FILE_INFO,
	/* 3 */ SM_CONFIRM_FILE,
	/* 4 */ SM_FADE_OUT_FILE_INFO,
	/* 5 */ SM_MOVE_FILE_TO_SLOT,
	/* 6 */ SM_FADE_OUT,
	/* 7 */ SM_LOAD_GAME
} SelectMode;

typedef enum {
	/* 0 */ FS_TITLE_SELECT_FILE, // "Please select a file."
	/* 1 */ FS_TITLE_OPEN_FILE,  // "Open this file?"
	/* 2 */ FS_TITLE_COPY_FROM,  // "Copy which file?"
	/* 3 */ FS_TITLE_COPY_TO,    // "Copy to which file?"
	/* 4 */ FS_TITLE_COPY_CONFIRM, // "Are you sure?"
	/* 5 */ FS_TITLE_COPY_COMPLETE, // "File copied."
	/* 6 */ FS_TITLE_ERASE_FILE, // "Erase which file?"
	/* 7 */ FS_TITLE_ERASE_CONFIRM, // "Are you sure?"
	/* 8 */ FS_TITLE_ERASE_COMPLETE // "File erased."
} TitleLabel;

typedef enum {
	/* -1 */ FS_WARNING_NONE = -1,
	/*  0 */ FS_WARNING_NO_FILE_COPY, // "No file to copy."
	/*  1 */ FS_WARNING_NO_FILE_ERASE, // "No file to erase."
	/*  2 */ FS_WARNING_NO_EMPTY_FILES, // "There is no empty file."
	/*  3 */ FS_WARNING_FILE_EMPTY, // "This is an empty file."
	/*  4 */ FS_WARNING_FILE_IN_USE // "This file is in use."
} WarningLabel;

typedef enum {
	/* 0 */ FS_BTN_MAIN_FILE_1,
	/* 1 */ FS_BTN_MAIN_FILE_2,
	/* 2 */ FS_BTN_MAIN_FILE_3,
	/* 3 */ FS_BTN_MAIN_COPY,
	/* 4 */ FS_BTN_MAIN_ERASE,
	/* 5 */ FS_BTN_MAIN_OPTIONS
} MainMenuButtonIndex;

typedef enum {
	/* 0 */ FS_BTN_COPY_FILE_1,
	/* 1 */ FS_BTN_COPY_FILE_2,
	/* 2 */ FS_BTN_COPY_FILE_3,
	/* 3 */ FS_BTN_COPY_QUIT
} CopyMenuButtonIndex;

typedef enum {
	/* 0 */ FS_BTN_ERASE_FILE_1,
	/* 1 */ FS_BTN_ERASE_FILE_2,
	/* 2 */ FS_BTN_ERASE_FILE_3,
	/* 3 */ FS_BTN_ERASE_QUIT
} EraseMenuButtonIndex;

typedef enum {
	/* 0 */ FS_BTN_SELECT_FILE_1,
	/* 1 */ FS_BTN_SELECT_FILE_2,
	/* 2 */ FS_BTN_SELECT_FILE_3,
	/* 3 */ FS_BTN_SELECT_YES,
	/* 4 */ FS_BTN_SELECT_QUIT
} SelectMenuButtonIndex;

typedef enum {
	/* 0 */ FS_BTN_CONFIRM_YES,
	/* 1 */ FS_BTN_CONFIRM_QUIT
} ConfirmButtonIndex;

typedef enum {
	/* 0 */ FS_BTN_ACTION_COPY,
	/* 1 */ FS_BTN_ACTION_ERASE
} ActionButtonIndex;

typedef enum {
	/* 0 */ FS_SETTING_AUDIO,
	/* 1 */ FS_SETTING_TARGET
} SettingIndex;

typedef enum {
	/* 0 */ FS_AUDIO_STEREO,
	/* 1 */ FS_AUDIO_MONO,
	/* 2 */ FS_AUDIO_HEADSET,
	/* 3 */ FS_AUDIO_SURROUND
} AudioOption;

typedef enum {
	/* 0 */ FS_CHAR_PAGE_HIRA,
	/* 1 */ FS_CHAR_PAGE_KATA,
	/* 2 */ FS_CHAR_PAGE_ENG
} CharPage;

typedef enum {
	/* 00 */ FS_KBD_BTN_HIRA,
	/* 01 */ FS_KBD_BTN_KATA,
	/* 02 */ FS_KBD_BTN_ENG,
	/* 03 */ FS_KBD_BTN_BACKSPACE,
	/* 04 */ FS_KBD_BTN_END,
	/* 99 */ FS_KBD_BTN_NONE = 99
} KeyboardButton;

void FileSelect_SetupCopySource(GameState* thisx);
void FileSelect_SelectCopySource(GameState* thisx);
void FileSelect_SetupCopyDest1(GameState* thisx);
void FileSelect_SetupCopyDest2(GameState* thisx);
void FileSelect_SelectCopyDest(GameState* thisx);
void FileSelect_ExitToCopySource1(GameState* thisx);
void FileSelect_ExitToCopySource2(GameState* thisx);
void FileSelect_SetupCopyConfirm1(GameState* thisx);
void FileSelect_SetupCopyConfirm2(GameState* thisx);
void FileSelect_CopyConfirm(GameState* thisx);
void FileSelect_ReturnToCopyDest(GameState* thisx);
void FileSelect_CopyAnim1(GameState* thisx);
void FileSelect_CopyAnim2(GameState* thisx);
void FileSelect_CopyAnim3(GameState* thisx);
void FileSelect_CopyAnim4(GameState* thisx);
void FileSelect_CopyAnim5(GameState* thisx);

void FileSelect_ExitCopyToMain(GameState* thisx);
void FileSelect_SetupEraseSelect(GameState* thisx);
void FileSelect_EraseSelect(GameState* thisx);
void FileSelect_SetupEraseConfirm1(GameState* thisx);
void FileSelect_SetupEraseConfirm2(GameState* thisx);
void FileSelect_EraseConfirm(GameState* thisx);
void FileSelect_ExitToEraseSelect1(GameState* thisx);
void FileSelect_ExitToEraseSelect2(GameState* thisx);
void FileSelect_EraseAnim1(GameState* thisx);
void FileSelect_EraseAnim2(GameState* thisx);
void FileSelect_EraseAnim3(GameState* thisx);
void FileSelect_ExitEraseToMain(GameState* thisx);

void FileSelect_UpdateKeyboardCursor(GameState* thisx);
void FileSelect_StartNameEntry(GameState* thisx);
void FileSelect_UpdateOptionsMenu(GameState* thisx);
void FileSelect_StartOptions(GameState* thisx);

void FileSelect_InitModeDraw(GameState* thisx);
void FileSelect_ConfigModeDraw(GameState* thisx);
void FileSelect_SelectModeDraw(GameState* thisx);

void FileSelect_PulsateCursor(GameState* thisx);
void FileSelect_DrawOptions(GameState* thisx);

void FileSelect_DrawNameEntry(GameState* thisx);
void FileSelect_DrawCharacter(GraphicsContext* gfxCtx, void* texture, s16 vtx);

extern s16 D_808123F0[];

extern u64 gFileSelConnectorTex;
extern u64 gFileSelForestMedallionTex;
extern u64 gFileSelFireMedallionTex;
extern u64 gFileSelWaterMedallionTex;
extern u64 gFileSelSpiritMedallionTex;
extern u64 gFileSelShadowMedallionTex;
extern u64 gFileSelLightMedallionTex;
extern u64 gFileSelWindow1Tex;
extern u64 gFileSelWindow2Tex;
extern u64 gFileSelWindow3Tex;
extern u64 gFileSelWindow4Tex;
extern u64 gFileSelWindow5Tex;
extern u64 gFileSelWindow6Tex;
extern u64 gFileSelWindow7Tex;
extern u64 gFileSelWindow8Tex;
extern u64 gFileSelWindow9Tex;
extern u64 gFileSelWindow10Tex;
extern u64 gFileSelWindow11Tex;
extern u64 gFileSelWindow12Tex;
extern u64 gFileSelWindow13Tex;
extern u64 gFileSelWindow14Tex;
extern u64 gFileSelWindow15Tex;
extern u64 gFileSelWindow16Tex;
extern u64 gFileSelWindow17Tex;
extern u64 gFileSelWindow18Tex;
extern u64 gFileSelWindow19Tex;
extern u64 gFileSelWindow20Tex;
extern u64 gFileSelKanjiButtonTex;
extern u64 gFileSelHiraganaButtonTex;
extern u64 gFileSelKatakanaButtonTex;
extern u64 gFileSelENDButtonENGTex;
extern u64 gFileSelENDButtonGERTex;
extern u64 gFileSelENDButtonFRATex;
extern u64 gFileSelBackspaceButtonTex;
extern u64 gFileSelNameBoxTex;
extern u64 gFileSelFileInfoBox1Tex;
extern u64 gFileSelFileInfoBox2Tex;
extern u64 gFileSelFileInfoBox3Tex;
extern u64 gFileSelFileInfoBox4Tex;
extern u64 gFileSelFileInfoBox5Tex;
extern u64 gFileSelDISKButtonTex;
extern u64 gFileSelOptionsDividerTex;
extern u64 gFileSelBrightnessCheckTex;
extern u64 gFileSelBigButtonHighlightTex;
extern u64 gFileSelCharHighlightTex;
extern u64 gFileSelMediumButtonHighlightTex;
extern u64 gFileSelSmallButtonHighlightTex;
extern u64 gFileSelKokiriEmeraldTex;
extern u64 gFileSelGoronRubyTex;
extern u64 gFileSelZoraSapphireTex;
extern u64 gFileSelNoFileToCopyENGTex;
extern u64 gFileSelNoFileToEraseENGTex;
extern u64 gFileSelNoEmptyFileENGTex;
extern u64 gFileSelFileEmptyENGTex;
extern u64 gFileSelFileInUseENGTex;
extern u64 gFileSelNoFileToCopyGERTex;
extern u64 gFileSelNoFileToEraseGERTex;
extern u64 gFileSelNoEmptyFileGERTex;
extern u64 gFileSelFileEmptyGERTex;
extern u64 gFileSelFileInUseGERTex;
extern u64 gFileSelNoFileToCopyFRATex;
extern u64 gFileSelNoFileToEraseFRATex;
extern u64 gFileSelNoEmptyFileFRATex;
extern u64 gFileSelFileEmptyFRATex;
extern u64 gFileSelFileInUseFRATex;
extern u64 gFileSelCopyWhichFileENGTex;
extern u64 gFileSelCopyToWhichFileENGTex;
extern u64 gFileSelAreYouSureENGTex;
extern u64 gFileSelFileCopiedENGTex;
extern u64 gFileSelWhichFile1GERTex;
extern u64 gFileSelCopyToWhichFileGERTex;
extern u64 gFileSelAreYouSureGERTex;
extern u64 gFileSelFileCopiedGERTex;
extern u64 gFileSelCopyWhichFileFRATex;
extern u64 gFileSelCopyToWhichFileFRATex;
extern u64 gFileSelAreYouSureFRATex;
extern u64 gFileSelFileCopiedFRATex;
extern u64 gFileSelPleaseSelectAFileENGTex;
extern u64 gFileSelOpenThisFileENGTex;
extern u64 gFileSelPleaseSelectAFileGERTex;
extern u64 gFileSelOpenThisFileGERTex;
extern u64 gFileSelPleaseSelectAFileFRATex;
extern u64 gFileSelOpenThisFileFRATex;
extern u64 gFileSelEraseWhichFileENGTex;
extern u64 gFileSelAreYouSure2ENGTex;
extern u64 gFileSelFileErasedENGTex;
extern u64 gFileSelWhichFile2GERTex;
extern u64 gFileSelAreYouSure2GERTex;
extern u64 gFileSelFileErasedGERTex;
extern u64 gFileSelEraseWhichFileFRATex;
extern u64 gFileSelAreYouSure2FRATex;
extern u64 gFileSelFileErasedFRATex;
extern u64 gFileSelOptionsENGTex;
extern u64 gFileSelOptionsGERTex;
extern u64 gFileSelNameENGTex;
extern u64 gFileSelNameGERTex;
extern u64 gFileSelNameFRATex;
extern u64 gFileSelControlsENGTex;
extern u64 gFileSelControlsGERTex;
extern u64 gFileSelControlsFRATex;
extern u64 gFileSelCopyButtonENGTex;
extern u64 gFileSelCopyButtonGERTex;
extern u64 gFileSelCopyButtonFRATex;
extern u64 gFileSelFile1ButtonENGTex;
extern u64 gFileSelFile2ButtonENGTex;
extern u64 gFileSelFile3ButtonENGTex;
extern u64 gFileSelFile1ButtonGERTex;
extern u64 gFileSelFile2ButtonGERTex;
extern u64 gFileSelFile3ButtonGERTex;
extern u64 gFileSelFile1ButtonFRATex;
extern u64 gFileSelFile2ButtonFRATex;
extern u64 gFileSelFile3ButtonFRATex;
extern u64 gFileSelYesButtonENGTex;
extern u64 gFileSelYesButtonGERTex;
extern u64 gFileSelYesButtonFRATex;
extern u64 gFileSelEraseButtonENGTex;
extern u64 gFileSelEraseButtonGERTex;
extern u64 gFileSelEraseButtonFRATex;
extern u64 gFileSelQuitButtonENGTex;
extern u64 gFileSelQuitButtonGERTex;
extern u64 gFileSelQuitButtonFRATex;
extern u64 gFileSelSurroundENGTex;
extern u64 gFileSelHeadsetENGTex;
extern u64 gFileSelHeadsetGERTex;
extern u64 gFileSelHeadsetFRATex;
extern u64 gFileSelMonoENGTex;
extern u64 gFileSelSOUNDENGTex;
extern u64 gFileSelSOUNDFRATex;
extern u64 gFileSelStereoENGTex;
extern u64 gFileSelStereoFRATex;
extern u64 gFileSelLTargetingENGTex;
extern u64 gFileSelLTargetingGERTex;
extern u64 gFileSelLTargetingFRATex;
extern u64 gFileSelSwitchENGTex;
extern u64 gFileSelSwitchGERTex;
extern u64 gFileSelSwitchFRATex;
extern u64 gFileSelHoldENGTex;
extern u64 gFileSelHoldGERTex;
extern u64 gFileSelHoldFRATex;
extern u64 gFileSelCheckBrightnessENGTex;
extern u64 gFileSelCheckBrightnessGERTex;
extern u64 gFileSelCheckBrightnessFRATex;
extern u64 gFileSelOptionsButtonENGTex;
extern u64 gFileSelOptionsButtonGERTex;
extern u64 gFileSelSaveXTex;
extern u64 gFileSelWindow1DL;
extern u64 gFileSelWindow2DL;
extern u64 gFileSelWindow3DL;

extern u64 gHeartEmptyTex;
extern u64 gHeartQuarterTex;
extern u64 gHeartHalfTex;
extern u64 gHeartThreeQuarterTex;
extern u64 gHeartFullTex;
extern u64 gDefenseHeartEmptyTex;
extern u64 gDefenseHeartQuarterTex;
extern u64 gDefenseHeartHalfTex;
extern u64 gDefenseHeartThreeQuarterTex;
extern u64 gDefenseHeartFullTex;
extern u64 gButtonBackgroundTex;
extern u64 gEquippedItemOutlineTex;
extern u64 gEmptyCLeftArrowTex;
extern u64 gEmptyCDownArrowTex;
extern u64 gEmptyCRightArrowTex;
extern u64 gSmallKeyCounterIconTex;
extern u64 gRupeeCounterIconTex;
extern u64 gClockIconTex;
extern u64 gCarrotIconTex;
extern u64 gMapDungeonEntranceIconTex;
extern u64 gMapChestIconTex;
extern u64 gArcheryScoreIconTex;
extern u64 gMapBossIconTex;
extern u64 gOcarinaATex;
extern u64 gOcarinaCDownTex;
extern u64 gOcarinaCRightTex;
extern u64 gOcarinaCLeftTex;
extern u64 gOcarinaCUpTex;
extern u64 gOcarinaTrebleClefTex;
extern u64 gNaviCUpJPTex;
extern u64 gNaviCUpENGTex;
extern u64 gCounterDigit0Tex;
extern u64 gCounterDigit1Tex;
extern u64 gCounterDigit2Tex;
extern u64 gCounterDigit3Tex;
extern u64 gCounterDigit4Tex;
extern u64 gCounterDigit5Tex;
extern u64 gCounterDigit6Tex;
extern u64 gCounterDigit7Tex;
extern u64 gCounterDigit8Tex;
extern u64 gCounterDigit9Tex;
extern u64 gCounterColonTex;
extern u64 gAmmoDigit0Tex;
extern u64 gAmmoDigit1Tex;
extern u64 gAmmoDigit2Tex;
extern u64 gAmmoDigit3Tex;
extern u64 gAmmoDigit4Tex;
extern u64 gAmmoDigit5Tex;
extern u64 gAmmoDigit6Tex;
extern u64 gAmmoDigit7Tex;
extern u64 gAmmoDigit8Tex;
extern u64 gAmmoDigit9Tex;
extern u64 gUnusedAmmoDigitHalfTex;
extern u64 gMagicBarEndTex;
extern u64 gMagicBarMidTex;
extern u64 gMagicBarFillTex;

#endif
