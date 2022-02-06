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