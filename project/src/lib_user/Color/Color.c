#include <z_lib_user.h>

static f32 hue2rgb(f32 p, f32 q, f32 t) {
	if (t < 0.0) t += 1;
	if (t > 1.0) t -= 1;
	if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
	if (t < 1.0 / 2.0) return q;
	if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
	
	return p;
}

void Color_ToHSL(Color_HSL* dest, Color_RGB8* src) {
	f32 r, g, b;
	f32 cmax, cmin, d;
	
	r = (f32)src->r / 255;
	g = (f32)src->g / 255;
	b = (f32)src->b / 255;
	
	cmax = Math_MaxF(r, (Math_MaxF(g, b)));
	cmin = Math_MinF(r, (Math_MinF(g, b)));
	dest->l = (cmax + cmin) / 2;
	d = cmax - cmin;
	
	if (cmax == cmin)
		dest->h = dest->s = 0;
	else {
		dest->s = dest->l > 0.5 ? d / (2 - cmax - cmin) : d / (cmax + cmin);
		
		if (cmax == r) {
			dest->h = (g - b) / d + (g < b ? 6 : 0);
		} else if (cmax == g) {
			dest->h = (b - r) / d + 2;
		} else if (cmax == b) {
			dest->h = (r - g) / d + 4;
		}
		dest->h /= 6.0;
	}
}

void Color_ToRGB(Color_RGB8* dest, Color_HSL* src) {
	if (src->s == 0) {
		dest->r = dest->g = dest->b = src->l;
	} else {
		f32 q = src->l < 0.5 ? src->l * (1 + src->s) : src->l + src->s - src->l * src->s;
		f32 p = 2.0 * src->l - q;
		dest->r = hue2rgb(p, q, src->h + 1.0 / 3.0) * 255;
		dest->g = hue2rgb(p, q, src->h) * 255;
		dest->b = hue2rgb(p, q, src->h - 1.0 / 3.0) * 255;
	}
}