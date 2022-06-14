#include <oot_mq_debug/z64hdr.h>

typedef struct {
	f32 h;
	f32 s;
	f32 l;
} Color_HSL;

typedef struct {
	f32 h;
	f32 s;
	f32 l;
	u8  alpha;
} Color_HSLA;

void Audio_PlaySys(u16 flag);
void Color_ToHSL(Color_HSL* dest, Color_RGB8* src);
void Color_ToRGB(Color_RGB8* dest, Color_HSL* src);

#ifdef DEV_BUILD
void Debug_Text(u32 rgba, s32 x, s32 y, char* fmt, ...);
void Debug_DmaLog(DmaRequest* req);
void DebugMenu_Update(PlayState* playState);
s32 DebugMenu_CineCamera(Camera* camera, Normal1* norm1, Player* player);
#else
#define Debug_Text(...)           ((void)0)
#define Debug_DmaLog(...)         ((void)0)
#define DebugMenu_Update(...)     ((void)0)
#define DebugMenu_CineCamera(...) 0
#endif

Vtx* Gui_AllocQuad(PlayState* playState, s16 x, s16 y, s16 width, s16 height, s16 u, s16 v);
f32 MaxF(f32 a, f32 b);
f32 MinF(f32 a, f32 b);
s16 MaxS(s16 a, s16 b);
s16 MinS(s16 a, s16 b);
f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
void SceneAnim_Update(PlayState* playState);