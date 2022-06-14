#ifndef __ULIB_MACROS_H__
#define __ULIB_MACROS_H__

#define CHK_ALL(AB, combo)      (~((gPlayState.state.input[0].AB.button) | ~(combo)) == 0)
#define CHK_ANY(AB, combo)      (((gPlayState.state.input[0].AB.button) & (combo)) != 0)
#define AVAL(base, type, value) ((type*)((u8*)base + value))

#define PRNT_DGRY "\e[90;2m"
#define PRNT_DRED "\e[91;2m"
#define PRNT_GRAY "\e[0;90m"
#define PRNT_REDD "\e[0;91m"
#define PRNT_GREN "\e[0;92m"
#define PRNT_YELW "\e[0;93m"
#define PRNT_BLUE "\e[0;94m"
#define PRNT_PRPL "\e[0;95m"
#define PRNT_CYAN "\e[0;96m"
#define PRNT_RSET "\e[m"

#define BinToMb(x) ((f32)(x) / (f32)0x100000)
#define BinToKb(x) ((f32)(x) / (f32)0x400)
#define MbToBin(x) (u32)(0x100000 * (x))
#define KbToBin(x) (u32)(0x400 * (x))

#ifndef DEV_BUILD

#define Assert(cond)  ((void)0)
#define osInfo(title) ((void)0)

#ifndef __ULIB_C__
#define osLibPrintf(...) ((void)0)
#endif

#else

#define Assert(cond)  ((cond) ? ((void)0) : __assert(#cond, __FUNCTION__, __LINE__))
#define osInfo(title) "" PRNT_GRAY "[" PRNT_REDD "%s" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]" PRNT_RSET ": " PRNT_REDD title, __FUNCTION__, __LINE__

#endif

#ifndef VIEW_ALL

#define VIEW_VIEWING                (1 << 0)
#define VIEW_VIEWPORT               (1 << 1)
#define VIEW_PROJECTION_PERSPECTIVE (1 << 2)
#define VIEW_PROJECTION_ORTHO       (1 << 3)
#define VIEW_ALL                    (VIEW_VIEWING | VIEW_VIEWPORT | VIEW_PROJECTION_PERSPECTIVE | VIEW_PROJECTION_ORTHO)

#define VIEW_FORCE_VIEWING                (VIEW_VIEWING << 4)
#define VIEW_FORCE_VIEWPORT               (VIEW_VIEWPORT << 4)
#define VIEW_FORCE_PROJECTION_PERSPECTIVE (VIEW_PROJECTION_PERSPECTIVE << 4)
#define VIEW_FORCE_PROJECTION_ORTHO       (VIEW_PROJECTION_ORTHO << 4)

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

#define View_SetPerspective      func_800AA460
#define View_Apply               View_Apply
#define View_ApplyTo             func_800AB9EC
#define View_UpdateViewingMatrix func_800AB944

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

#define CAM_DEG_TO_BINANG(degrees) (s16)((degrees) * 182.04167f + .5f)
#define CAM_BINANG_TO_DEG(binang)  ((f32)(binang) * (360.0001525f / 65535.0f))

#define PAUSE_EQUIP_PLAYER_WIDTH  64
#define PAUSE_EQUIP_PLAYER_HEIGHT 112

#define PAUSE_EQUIP_BUFFER_SIZE                        sizeof(u16[PAUSE_EQUIP_PLAYER_HEIGHT][PAUSE_EQUIP_PLAYER_WIDTH])
#define PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE 0x5000

#endif

#endif