#include <oot_mq_debug/z64hdr.h>

Vec3f Vec_Multiply(Vec3f* a, Vec3f* b) {
	return (Vec3f) {
		       a->x* b->x,
			       a->y* b->y,
			       a->z* a->z
	};
}