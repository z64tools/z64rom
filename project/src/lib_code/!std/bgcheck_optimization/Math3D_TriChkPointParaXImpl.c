#include <uLib.h>

/*
   z64ram = 0x800CD34C
   z64rom = 0xB444EC
   z64next = 0x800CD668
 */

s32 OldMath3D_TriChkPointParaXImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 y, f32 z, f32 detMax, f32 chkDist, f32 nx);

s32 Math3D_TriChkPointParaXImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 y, f32 z, f32 detMax, f32 chkDist, f32 nx) {
	s32 ret;
	
	Profiler_Start(&gLibCtx.profiler.math3D);
	
	if (gLibCtx.state.newMath3D)
		ret = NewMath3D_TriChkPointParaXImpl(v0, v1, v2, y, z, detMax, chkDist, nx);
	
	else
		ret = OldMath3D_TriChkPointParaXImpl(v0, v1, v2, y, z, detMax, chkDist, nx);
	
	Profiler_End(&gLibCtx.profiler.math3D);
	
	return ret;
}