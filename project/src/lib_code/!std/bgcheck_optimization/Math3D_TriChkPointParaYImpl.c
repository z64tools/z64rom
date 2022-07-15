#include <uLib.h>
#include "code/sys_math3d.h"

/*
   z64ram = 0x800CCBE4
   z64rom = 0xB43D84
   z64next = 0x800CCF00
 */

s32 OldMath3D_TriChkPointParaYImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 z, f32 x, f32 detMax, f32 chkDist, f32 ny);

s32 Math3D_TriChkPointParaYImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 z, f32 x, f32 detMax, f32 chkDist, f32 ny) {
	s32 ret;
	
	Profiler_Start(&gLibCtx.profiler.math3D);
	
	if (gLibCtx.state.newMath3D)
		ret = NewMath3D_TriChkPointParaYImpl(v0, v1, v2, z, x, detMax, chkDist, ny);
	
	else
		ret = OldMath3D_TriChkPointParaYImpl(v0, v1, v2, z, x, detMax, chkDist, ny);
	
	Profiler_End(&gLibCtx.profiler.math3D);
	
	return ret;
}