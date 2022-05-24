#include <uLib.h>

f32 MaxF(f32 a, f32 b) {
	return a >= b ? a : b;
}

f32 MinF(f32 a, f32 b) {
	return a < b ? a : b;
}

s16 MaxS(s16 a, s16 b) {
	return a >= b ? a : b;
}

s16 MinS(s16 a, s16 b) {
	return a < b ? a : b;
}

f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
	f32 a = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
	f32 b = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
	f32 c = (x1 - xm1) * 0.5f;
	
	return (((((a * k) + b) * k) + c) * k) + x0;
}

f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
	f32 coeff[4];
	
	coeff[0] = (1.0f - k) * (1.0f - k) * (1.0f - k) / 6.0f;
	coeff[1] = k * k * k / 2.0f - k * k + 2.0f / 3.0f;
	coeff[2] = -k * k * k / 2.0f + k * k / 2.0f + k / 2.0f + 1.0f / 6.0f;
	coeff[3] = k * k * k / 6.0f;
	
	return (coeff[0] * xm1) + (coeff[1] * x0) + (coeff[2] * x1) + (coeff[3] * x2);
}