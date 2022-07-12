
/* Allow use of 64-bit instructions */
__asm__ (".set mips3");

/* Function to convert a 64-bit value stored in two registers
   to instead be stored in a single register */
static long long to_64b(long long a) {
	register long long ret;
	long hi = a >> 32;
	long lo = a << 32 >> 32;
	
	/* TODO try and rewrite without loads/stores */
	__asm__ (
		"sw  %[hi], 0($sp);"
		"sw  %[lo], 4($sp);"
		"ld  %[ret], ($sp);"
		: [ret] "=r" (ret)
		: [hi]  "r" (hi),
		[lo]  "r" (lo)
	);
	
	return ret;
}

/* Function to convert a 64-bit value stored in one register
   to instead be stored in two registers */
static long long to_32b(long long a) {
	register long long ret;
	
	/* This is sketchy but idk if there's a way to
	   tell it to put %[ret] in both $v0 and $v1 */
	__asm__ (
		"dsll32  $v1, %[a], 0;"
		"dsra32  $v1, $v1, 0;"
		"dsra32  %[ret], %[a], 0;"
		: [ret] "=r" (ret)
		: [a]  "r" (a)
	);
	
	return ret;
}

unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
	unsigned long long ret;
	unsigned long long r1 = to_64b(a);
	unsigned long long r2 = to_64b(b);
	
	__asm__ (
		"dremu   %[ret], %[r1], %[r2];"
		: [ret] "=r" (ret)
		: [r1] "r" (r1),
		[r2] "r" (r2)
	);
	
	return to_32b(ret);
}

unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
	unsigned long long ret;
	unsigned long long r1 = to_64b(a);
	unsigned long long r2 = to_64b(b);
	
	__asm__ (
		"ddivu   %[ret], %[r1], %[r2];"
		: [ret] "=r" (ret)
		: [r1] "r" (r1),
		[r2] "r" (r2)
	);
	
	return to_32b(ret);
}

signed long long __moddi3(signed long long a, signed long long b) {
	signed long long ret;
	signed long long r1 = to_64b(a);
	signed long long r2 = to_64b(b);
	
	__asm__ (
		"drem    %[ret], %[r1], %[r2];"
		: [ret] "=r" (ret)
		: [r1] "r" (r1),
		[r2] "r" (r2)
	);
	
	return to_32b(ret);
}

signed long long __divdi3(signed long long a, signed long long b) {
	signed long long ret;
	signed long long r1 = to_64b(a);
	signed long long r2 = to_64b(b);
	
	__asm__ (
		"ddiv    %[ret], %[r1], %[r2];"
		: [ret] "=r" (ret)
		: [r1] "r" (r1),
		[r2] "r" (r2)
	);
	
	return to_32b(ret);
}

#define FLT_MANT_DIG 24
#define CHAR_BIT     8

typedef union {
	unsigned int u;
	float f;
} float_bits;

typedef union {
	signed long long all;
	struct {
#if _YUGA_LITTLE_ENDIAN
		unsigned int low;
		signed int   high;
#else
		signed int   high;
		unsigned int low;
#endif // _YUGA_LITTLE_ENDIAN
	} s;
} dwords;

unsigned long long __fixunssfdi(float a) {
	if (a <= 0.0f)
		return 0;
	double da = a;
	unsigned int high = da / 4294967296.f; // da / 0x1p32f;
	unsigned int low = da - (double)high * 4294967296.f; // high * 0x1p32f;
	
	return ((unsigned long long)high << 32) | low;
}

int __clzsi2(signed int a) {
	unsigned int x = (unsigned int)a;
	signed int t = ((x & 0xFFFF0000) == 0) << 4; // if (x is small) t = 16 else 0
	
	x >>= 16 - t;          // x = [0 - 0xFFFF]
	unsigned int r = t;    // r = [0, 16]
	
	// return r + clz(x)
	t = ((x & 0xFF00) == 0) << 3;
	x >>= 8 - t; // x = [0 - 0xFF]
	r += t;                // r = [0, 8, 16, 24]
	// return r + clz(x)
	t = ((x & 0xF0) == 0) << 2;
	x >>= 4 - t; // x = [0 - 0xF]
	r += t;                // r = [0, 4, 8, 12, 16, 20, 24, 28]
	// return r + clz(x)
	t = ((x & 0xC) == 0) << 1;
	x >>= 2 - t; // x = [0 - 3]
	r += t;                // r = [0 - 30] and is even
	
	return r + ((2 - x) & -((x & 2) == 0));
}

int __clzdi2(signed long long a) {
	dwords x;
	
	x.all = a;
	const signed int f = -(x.s.high == 0);
	
	return __builtin_clz((x.s.high & ~f) | (x.s.low & f)) +
	       (f & ((signed int)(sizeof(signed int) * CHAR_BIT)));
}

float __floatundisf(unsigned long long a) {
	if (a == 0)
		return 0.0F;
	const unsigned N = sizeof(unsigned long long) * CHAR_BIT;
	int sd = N - __builtin_clzll(a); // number of significant digits
	int e = sd - 1;        // 8 exponent
	
	if (sd > FLT_MANT_DIG) {
		//  start:  0000000000000000000001xxxxxxxxxxxxxxxxxxxxxxPQxxxxxxxxxxxxxxxxxx
		//  finish: 000000000000000000000000000000000000001xxxxxxxxxxxxxxxxxxxxxxPQR
		//                                                12345678901234567890123456
		//  1 = msb 1 bit
		//  P = bit FLT_MANT_DIG-1 bits to the right of 1
		//  Q = bit FLT_MANT_DIG bits to the right of 1
		//  R = "or" of all bits to the right of Q
		switch (sd) {
			case FLT_MANT_DIG + 1:
				a <<= 1;
				break;
			case FLT_MANT_DIG + 2:
				break;
			default:
				a = (a >> (sd - (FLT_MANT_DIG + 2))) |
					((a & ((unsigned long long)(-1) >> ((N + FLT_MANT_DIG + 2) - sd))) != 0);
		}
		;
		// finish:
		a |= (a & 4) != 0; // Or P into R
		++a;           // round - this step may add a significant bit
		a >>= 2;       // dump Q and R
		// a is now rounded to FLT_MANT_DIG or FLT_MANT_DIG+1 bits
		if (a & ((unsigned long long)1 << FLT_MANT_DIG)) {
			a >>= 1;
			++e;
		}
		// a is now rounded to FLT_MANT_DIG bits
	} else {
		a <<= (FLT_MANT_DIG - sd);
		// a is now rounded to FLT_MANT_DIG bits
	}
	float_bits fb;
	
	fb.u = ((e + 127) << 23) | // exponent
		((unsigned int)a & 0x007FFFFF); // mantissa
	
	return fb.f;
}
