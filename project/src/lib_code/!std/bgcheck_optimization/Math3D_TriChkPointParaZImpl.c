#include <uLib.h>
#include "code/sys_math3d.h"

/*
   z64ram = 0x800CD9D0
   z64rom = 0xB44B70
   z64next = 0x800CDD18
 */

s32 OldMath3D_TriChkPointParaZImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 x, f32 y, f32 detMax, f32 chkDist, f32 nz);

s32 Math3D_TriChkPointParaZImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 x, f32 y, f32 detMax, f32 chkDist, f32 nz) {
	s32 ret;
	
	// OSTime start;
	//
	// if (gLibCtx.profiler.enabled)
	// 	start = osGetTime();
	
	if (gLibCtx.state.newMath3D)
		ret = NewMath3D_TriChkPointParaZImpl(v0, v1, v2, x, y, detMax, chkDist, nz);
	
	else
		ret = OldMath3D_TriChkPointParaZImpl(v0, v1, v2, x, y, detMax, chkDist, nz);
	
	// if (gLibCtx.profiler.enabled)
	// 	gLibCtx.profiler.newMath3D[2].buffer[gLibCtx.profiler.newMath3D[2].ringId % 20] += osGetTime() - start;
	
	return ret;
}