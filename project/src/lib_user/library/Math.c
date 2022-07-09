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

Vec3f Math_Vec3f_Spline1(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2) {
	//crustify
	return (Vec3f) {
       Math_Spline1(k, xm1.x, x0.x, x1.x, x2.x),
       Math_Spline1(k, xm1.y, x0.y, x1.y, x2.y),
       Math_Spline1(k, xm1.z, x0.z, x1.z, x2.z),
	};
	//uncrustify
}

Vec3f Math_Vec3f_Spline2(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2) {
	//crustify
	return (Vec3f) {
       Math_Spline2(k, xm1.x, x0.x, x1.x, x2.x),
       Math_Spline2(k, xm1.y, x0.y, x1.y, x2.y),
       Math_Spline2(k, xm1.z, x0.z, x1.z, x2.z),
	};
	//uncrustify
}

Vec3f Math_Vec3f_YawDist(f32 dist, s16 yaw) {
	Vec3f ret = { 0 };
	
	ret.x += Math_SinS(yaw) * dist;
	ret.z += Math_CosS(yaw) * dist;
	
	return ret;
}

Vec3f Math_Vec3f_YawPitchDist(f32 dist, s16 yaw, s16 pitch) {
	Vec3f ret = { 0 };
	
	ret.x += Math_SinS(yaw) * dist;
	ret.z += Math_CosS(yaw) * dist;
	ret.y += Math_SinS(-pitch) * dist;
	
	return ret;
}

Vec3f Math_Vec3f_PosRelativeTo(Vec3f* target, Vec3f* origin, s16 originYaw) {
	f32 cosRot2Y;
	f32 sinRot2Y;
	f32 deltaX;
	f32 deltaZ;
	
	cosRot2Y = Math_CosS(originYaw);
	sinRot2Y = Math_SinS(originYaw);
	deltaX = target->x - origin->x;
	deltaZ = target->z - origin->z;
	
	//crustify
	return (Vec3f) {
		.x = (deltaX * cosRot2Y) - (deltaZ * sinRot2Y),
		.z = (deltaX * sinRot2Y) + (deltaZ * cosRot2Y),
		.y = target->y - origin->y
	};
	//uncrustify
}

f32 PowF(f32 a, f32 b) {
	if (b == 1.0f) return a;
	if (b == 2.0f) return a * a;
	if (b == -1.0f) return 1.0f / a;
	if (b == 0.0f) return 1.0f;
	
	f32 low, high, sqr, acc, mid;
	
	if (b >= 1) {
		sqr = PowF(a, b / 2);
		
		return sqr * sqr;
	} else {
		low = 0.0f;
		high = 1.0f;
		sqr = sqrtf(a);
		acc = sqr;
		mid = high / 2;
		
		while (ABS(mid - b) > 0.000001f) {
			sqr = sqrtf(sqr);
			
			if (mid <= b) {
				low = mid;
				acc *= sqr;
			} else {
				high = mid;
				acc *= (1 / sqr);
			}
			mid = (low + high) / 2;
		}
		
		return acc;
	}
}