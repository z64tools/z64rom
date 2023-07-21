#include <uLib.h>
#include <uLib_math.h>

static f32 hue2rgb(f32 p, f32 q, f32 t) {
    if (t < 0.0) t += 1;
    if (t > 1.0) t -= 1;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    
    return p;
}

Color_HSL Color_RgbToHsl(f32 r, f32 g, f32 b) {
    Color_HSL dest;
    f32 cmax, cmin, d;
    
    cmax = MaxF(r, (MaxF(g, b)));
    cmin = MinF(r, (MinF(g, b)));
    dest.l = (cmax + cmin) / 2;
    d = cmax - cmin;
    
    if (cmax == cmin)
        dest.h = dest.s = 0;
    else {
        dest.s = dest.l > 0.5 ? d / (2 - cmax - cmin) : d / (cmax + cmin);
        
        if (cmax == r) {
            dest.h = (g - b) / d + (g < b ? 6 : 0);
        } else if (cmax == g) {
            dest.h = (b - r) / d + 2;
        } else if (cmax == b) {
            dest.h = (r - g) / d + 4;
        }
        dest.h /= 6.0;
    }
    
    return dest;
}

Color_RGB8 Color_HslToRgb(f32 h, f32 s, f32 l) {
    Color_RGB8 dest;
    
    if (s == 0) {
        dest.r = dest.g = dest.b = l * 255;
    } else {
        f32 q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        f32 p = 2.0 * l - q;
        dest.r = hue2rgb(p, q, h + 1.0 / 3.0) * 255;
        dest.g = hue2rgb(p, q, h) * 255;
        dest.b = hue2rgb(p, q, h - 1.0 / 3.0) * 255;
    }
    
    return dest;
}
