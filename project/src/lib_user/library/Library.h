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

void Color_ToHSL(Color_HSL* dest, Color_RGB8* src);
void Color_ToRGB(Color_RGB8* dest, Color_HSL* src);
void DebugMenu_DebugText(u32 rgba, s32 x, s32 y, char* fmt, ...);
void DebugMenu_Update(GlobalContext* globalCtx);
Vtx* Gui_AllocQuad(GlobalContext* globalCtx, s16 x, s16 y, s16 width, s16 height, s16 u, s16 v);
f32 MaxF(f32 a, f32 b);
f32 MinF(f32 a, f32 b);
s16 MaxS(s16 a, s16 b);
s16 MinS(s16 a, s16 b);
f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
void SceneAnim_Update(GlobalContext* globalCtx);