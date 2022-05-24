#include <uLib.h>

u32 __mib_DebugMenuC;

#ifdef DEV_BUILD

typedef void (* PageFunc)(GlobalContext*);

typedef enum {
	DEBUGSYS_PAGE_MAIN,
	DEBUGSYS_PAGE_MINIMAP
} DebugPage;

typedef struct {
	struct {
		u8 on : 1;
	} state;
	DebugPage page;
	DebugPage setPage;
	Vtx* vtx;
	bool hitViewEnabled;
	bool colViewEnabled;
	bool cineModeEnabled;
} DebugState;

typedef struct {
	PageFunc func;
	char*    name;
	u8 playerFreeze;
} DebugPageInfo;

extern Arena sZeldaArena;
asm ("sZeldaArena = 0x8015FF80");

static DebugState sDebugMenuCtx;
static Gfx sPolyGfxInit[] = {
	gsSPLoadGeometryMode(G_ZBUFFER | G_SHADE | G_LIGHTING),
	gsSPTexture(0, 0, 0, G_TX_RENDERTILE, G_OFF),
	gsDPPipeSync(),
	gsDPSetCycleType(G_CYC_1CYCLE),
	gsDPSetRenderMode(
		Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_DEC | GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
		Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_DEC | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
	),
	gsDPSetCombineLERP(PRIMITIVE, 0, SHADE, 0, 0, 0, 0, ENVIRONMENT, PRIMITIVE, 0, SHADE, 0, 0, 0, 0, ENVIRONMENT),
	gsDPSetEnvColor(255, 255, 255, 128),
	gsSPEndDisplayList(),
};

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static Vec3f DebugMenu_TriNorm(Vec3f* v1, Vec3f* v2, Vec3f* v3) {
	Vec3f norm;
	
	norm.x = (v2->y - v1->y) * (v3->z - v1->z) - (v2->z - v1->z) * (v3->y - v1->y);
	norm.y = (v2->z - v1->z) * (v3->x - v1->x) - (v2->x - v1->x) * (v3->z - v1->z);
	norm.z = (v2->x - v1->x) * (v3->y - v1->y) - (v2->y - v1->y) * (v3->x - v1->x);
	
	f32 mag = sqrtf(SQXYZ(norm));
	
	if (mag != 0.0f) {
		norm.x *= 127.0f / mag;
		norm.y *= 127.0f / mag;
		norm.z *= 127.0f / mag;
	}
	
	return norm;
}

static void DebugMenu_DrawQuad3D(GlobalContext* globalCtx, Gfx** gfxP, Vec3f* v1, Vec3f* v2, Vec3f* v3, Vec3f* v4) {
	Vtx* v = Graph_Alloc(globalCtx->state.gfxCtx, 4 * sizeof(Vtx));
	Vec3f norm = DebugMenu_TriNorm(v1, v2, v4);
	
	v[0] = gdSPDefVtxN(v1->x, v1->y, v1->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	v[1] = gdSPDefVtxN(v2->x, v2->y, v2->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	v[2] = gdSPDefVtxN(v3->x, v3->y, v3->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	v[3] = gdSPDefVtxN(v4->x, v4->y, v4->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	
	gSPVertex((*gfxP)++, v, 4, 0);
	gSP2Triangles((*gfxP)++, 0, 1, 2, 0, 0, 2, 3, 0);
}

static void DebugMenu_DrawTri3D(GlobalContext* globalCtx, Gfx** gfxP, Vec3f* v1, Vec3f* v2, Vec3f* v3) {
	Vtx* v = Graph_Alloc(globalCtx->state.gfxCtx, 3 * sizeof(Vtx));
	Vec3f norm = DebugMenu_TriNorm(v1, v2, v3);
	
	v[0] = gdSPDefVtxN(v1->x, v1->y, v1->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	v[1] = gdSPDefVtxN(v2->x, v2->y, v2->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	v[2] = gdSPDefVtxN(v3->x, v3->y, v3->z, 0, 0, norm.x, norm.y, norm.z, 0xFF);
	
	gSPVertex((*gfxP)++, v, 3, 0);
	gSP1Triangle((*gfxP)++, 0, 1, 2, 0);
}

static void DebugMenu_DrawCyl(GlobalContext* globalCtx, Gfx** gfxP, Vec3s* pos, s16 radius, s16 height) {
	static Gfx* pCylGfx = NULL;
	
	if (pCylGfx == NULL) {
		#define CYL_DIVS 12
		static Gfx cylGfx[5 + CYL_DIVS * 2];
		static Vtx cylVtx[2 + CYL_DIVS * 2];
		
		Gfx* cylGfxP = cylGfx;
		
		cylVtx[0] = gdSPDefVtxN(0, 0,   0, 0, 0, 0, -127, 0, 0xFF);
		cylVtx[1] = gdSPDefVtxN(0, 128, 0, 0, 0, 0, 127,  0, 0xFF);
		
		for (s32 i = 0; i < CYL_DIVS; ++i) {
			s32 vtxX = Math_FFloorF(0.5f + cosf(2.0f * M_PI * i / CYL_DIVS) * 128.0f);
			s32 vtxZ = Math_FFloorF(0.5f - sinf(2.0f * M_PI * i / CYL_DIVS) * 128.0f);
			s32 normX = cosf(2.0f * M_PI * i / CYL_DIVS) * 127.0f;
			s32 normZ = -sinf(2.0f * M_PI * i / CYL_DIVS) * 127.0f;
			
			cylVtx[2 + i * 2 + 0] = gdSPDefVtxN(
				vtxX,
				0,
				vtxZ,
				0,
				0,
				normX,
				0,
				normZ,
				0xFF
			);
			cylVtx[2 + i * 2 + 1] = gdSPDefVtxN(
				vtxX,
				128,
				vtxZ,
				0,
				0,
				normX,
				0,
				normZ,
				0xFF
			);
		}
		
		gSPSetGeometryMode(cylGfxP++, G_CULL_BACK | G_SHADING_SMOOTH);
		
		gSPVertex(cylGfxP++, cylVtx, 2 + CYL_DIVS * 2, 0);
		for (s32 i = 0; i < CYL_DIVS; ++i) {
			s32 p = (i + CYL_DIVS - 1) % CYL_DIVS;
			
			gSP2Triangles(
				cylGfxP++,
				2 + p * 2 + 0,
				2 + i * 2 + 0,
				2 + i * 2 + 1,
				0,
				2 + p * 2 + 0,
				2 + i * 2 + 1,
				2 + p * 2 + 1,
				0
			);
		}
		
		gSPClearGeometryMode(cylGfxP++, G_SHADING_SMOOTH);
		for (s32 i = 0; i < CYL_DIVS; ++i) {
			s32 p = (i + CYL_DIVS - 1) % CYL_DIVS;
			
			gSP2Triangles(
				cylGfxP++,
				0,
				2 + i * 2 + 0,
				2 + p * 2 + 0,
				0,
				1,
				2 + p * 2 + 1,
				2 + i * 2 + 1,
				0
			);
		}
		
		gSPClearGeometryMode(cylGfxP++, G_CULL_BACK);
		gSPEndDisplayList(cylGfxP++);
#undef CYL_DIVS
		
		pCylGfx = cylGfx;
	}
	
	Matrix_Push();
	Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
	Matrix_Scale(radius / 128.0f, height / 128.0f, radius / 128.0f, MTXMODE_APPLY);
	
	gSPMatrix(
		(*gfxP)++,
		Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__),
		G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH
	);
	gSPDisplayList((*gfxP)++, pCylGfx);
	gSPPopMatrix((*gfxP)++, G_MTX_MODELVIEW);
	
	Matrix_Pop();
}

static Vec3f DebugMenu_IcoSphSubdivideEdge(Vec3f* a, Vec3f* b) {
	f32 mag;
	Vec3f ret;
	
	ret.x = a->x + b->x;
	ret.y = a->y + b->y;
	ret.z = a->z + b->z;
	mag = sqrtf(SQXYZ(ret));
	ret.x *= 1.0f / mag;
	ret.y *= 1.0f / mag;
	ret.z *= 1.0f / mag;
	
	return ret;
}

static void DebugMenu_DrawSph(GlobalContext* globalCtx, Gfx** gfxP, Vec3s* pos, s16 radius) {
	static Gfx* pSphGfx = NULL;
	static Vtx sphVtx[42];
	static Gfx sphGfx[45];
	
	Gfx* sphGfxP;
	s32 i;
	
	if (pSphGfx == NULL) {
		Vec3f vtx[42];
		s32 r0n = 1, r0m = r0n / 5, r0i = 0 + 0;
		s32 r1n = 5, r1m = r1n / 5, r1i = r0i + r0n;
		s32 r2n = 10, r2m = r2n / 5, r2i = r1i + r1n;
		s32 r3n = 10, r3m = r3n / 5, r3i = r2i + r2n;
		s32 r4n = 10, r4m = r4n / 5, r4i = r3i + r3n;
		s32 r5n = 5, r5m = r5n / 5, r5i = r4i + r4n;
		s32 r6n = 1, r6m = r6n / 5, r6i = r5i + r5n;
		
		vtx[r0i + (0 * r0m + 0) % r0n] = (Vec3f) { 0.0f,  1.0f, 0.0f };
		vtx[r6i + (0 * r6m + 0) % r6n] = (Vec3f) { 0.0f, -1.0f, 0.0f };
		
		for (i = 0; i < 5; ++i) {
			static const f32 aXZ = 2.0f * M_PI / 10.0f;
			static const f32 aY = 0.463647609f; // Math_FAtanF(1.0f / 2.0f);
			
			vtx[r2i + (i * r2m + 0) % r2n] = (Vec3f) {
				cosf(aXZ * (i * r2m + 0)) * cosf(aY * 1.0f),
				sinf(aY * 1.0f),
				-sinf(aXZ * (i * r2m + 0)) * cosf(aY * 1.0f),
			};
			vtx[r4i + (i * r4m + 0) % r4n] = (Vec3f) {
				cosf(aXZ * (i * r4m + 1)) * cosf(aY * -1.0f),
				sinf(aY * -1.0f),
				-sinf(aXZ * (i * r4m + 1)) * cosf(aY * -1.0f),
			};
		}
		
		for (i = 0; i < 5; ++i) {
			vtx[r1i + (i * r1m + 0) % r1n] =
				DebugMenu_IcoSphSubdivideEdge(&vtx[r0i + (i * r0m + 0) % r0n], &vtx[r2i + (i * r2m + 0) % r2n]);
			vtx[r2i + (i * r2m + 1) % r2n] =
				DebugMenu_IcoSphSubdivideEdge(&vtx[r2i + (i * r2m + 0) % r2n], &vtx[r2i + (i * r2m + 2) % r2n]);
			vtx[r3i + (i * r3m + 0) % r3n] =
				DebugMenu_IcoSphSubdivideEdge(&vtx[r2i + (i * r2m + 0) % r2n], &vtx[r4i + (i * r4m + 0) % r4n]);
			vtx[r3i + (i * r3m + 1) % r3n] =
				DebugMenu_IcoSphSubdivideEdge(&vtx[r4i + (i * r4m + 0) % r4n], &vtx[r2i + (i * r2m + 2) % r2n]);
			vtx[r4i + (i * r4m + 1) % r4n] =
				DebugMenu_IcoSphSubdivideEdge(&vtx[r4i + (i * r4m + 0) % r4n], &vtx[r4i + (i * r4m + 2) % r4n]);
			vtx[r5i + (i * r5m + 0) % r5n] =
				DebugMenu_IcoSphSubdivideEdge(&vtx[r4i + (i * r4m + 0) % r4n], &vtx[r6i + (i * r6m + 0) % r6n]);
		}
		
		for (i = 0; i < 42; ++i) {
			sphVtx[i] = gdSPDefVtxN(
				Math_FFloorF(0.5f + vtx[i].x * 128.0f),
				Math_FFloorF(0.5f + vtx[i].y * 128.0f),
				Math_FFloorF(0.5f + vtx[i].z * 128.0f),
				0,
				0,
				vtx[i].x * 127.0f,
				vtx[i].y * 127.0f,
				vtx[i].z * 127.0f,
				0xFF
			);
		}
		
		pSphGfx = sphGfxP = sphGfx;
		
		gSPSetGeometryMode(sphGfxP++, G_CULL_BACK | G_SHADING_SMOOTH);
		gSPVertex(sphGfxP++, &sphVtx[r0i], r0n + r1n + r2n + r3n, r0i - r0i);
		
		r3i -= r0i;
		r2i -= r0i;
		r1i -= r0i;
		r0i -= r0i;
		
		for (i = 0; i < 5; ++i) {
			s32 v[24];
			
			v[0] = r0i + (i * r0m + 0) % r0n;
			v[1] = r1i + (i * r1m + 0) % r1n;
			v[2] = r1i + (i * r1m + 1) % r1n;
			v[3] = r1i + (i * r1m + 0) % r1n;
			v[4] = r2i + (i * r2m + 0) % r2n;
			v[5] = r2i + (i * r2m + 1) % r2n;
			v[6] = r1i + (i * r1m + 0) % r1n;
			v[7] = r2i + (i * r2m + 1) % r2n;
			v[8] = r1i + (i * r1m + 1) % r1n;
			v[9] = r1i + (i * r1m + 1) % r1n;
			v[10] = r2i + (i * r2m + 1) % r2n;
			v[11] = r2i + (i * r2m + 2) % r2n;
			v[12] = r2i + (i * r2m + 0) % r2n;
			v[13] = r3i + (i * r3m + 0) % r3n;
			v[14] = r2i + (i * r2m + 1) % r2n;
			v[15] = r2i + (i * r2m + 1) % r2n;
			v[16] = r3i + (i * r3m + 0) % r3n;
			v[17] = r3i + (i * r3m + 1) % r3n;
			v[18] = r2i + (i * r2m + 1) % r2n;
			v[19] = r3i + (i * r3m + 1) % r3n;
			v[20] = r2i + (i * r2m + 2) % r2n;
			v[21] = r2i + (i * r2m + 2) % r2n;
			v[22] = r3i + (i * r3m + 1) % r3n;
			v[23] = r3i + (i * r3m + 2) % r3n;
			
			gSP2Triangles(sphGfxP++, v[0], v[1], v[2], 0, v[3], v[4], v[5], 0);
			gSP2Triangles(sphGfxP++, v[6], v[7], v[8], 0, v[9], v[10], v[11], 0);
			gSP2Triangles(sphGfxP++, v[12], v[13], v[14], 0, v[15], v[16], v[17], 0);
			gSP2Triangles(sphGfxP++, v[18], v[19], v[20], 0, v[21], v[22], v[23], 0);
		}
		
		gSPVertex(sphGfxP++, &sphVtx[r4i], r4n + r5n + r6n, r4i - r4i);
		
		r6i -= r4i;
		r5i -= r4i;
		r4i -= r4i;
		
		for (i = 0; i < 5; ++i) {
			s32 v[24];
			
			v[0] = r3i + (i * r3m + 1) % r3n;
			v[1] = r4i + (i * r4m + 0) % r4n;
			v[2] = r4i + (i * r4m + 1) % r4n;
			v[3] = r3i + (i * r3m + 1) % r3n;
			v[4] = r4i + (i * r4m + 1) % r4n;
			v[5] = r3i + (i * r3m + 2) % r3n;
			v[6] = r3i + (i * r3m + 2) % r3n;
			v[7] = r4i + (i * r4m + 1) % r4n;
			v[8] = r4i + (i * r4m + 2) % r4n;
			v[9] = r3i + (i * r3m + 2) % r3n;
			v[10] = r4i + (i * r4m + 2) % r4n;
			v[11] = r3i + (i * r3m + 3) % r3n;
			v[12] = r4i + (i * r4m + 0) % r4n;
			v[13] = r5i + (i * r5m + 0) % r5n;
			v[14] = r4i + (i * r4m + 1) % r4n;
			v[15] = r4i + (i * r4m + 1) % r4n;
			v[16] = r5i + (i * r5m + 0) % r5n;
			v[17] = r5i + (i * r5m + 1) % r5n;
			v[18] = r4i + (i * r4m + 1) % r4n;
			v[19] = r5i + (i * r5m + 1) % r5n;
			v[20] = r4i + (i * r4m + 2) % r4n;
			v[21] = r5i + (i * r5m + 0) % r5n;
			v[22] = r6i + (i * r6m + 0) % r6n;
			v[23] = r5i + (i * r5m + 1) % r5n;
			
			gSP2Triangles(sphGfxP++, v[0], v[1], v[2], 0, v[3], v[4], v[5], 0);
			gSP2Triangles(sphGfxP++, v[6], v[7], v[8], 0, v[9], v[10], v[11], 0);
			gSP2Triangles(sphGfxP++, v[12], v[13], v[14], 0, v[15], v[16], v[17], 0);
			gSP2Triangles(sphGfxP++, v[18], v[19], v[20], 0, v[21], v[22], v[23], 0);
		}
		gSPClearGeometryMode(sphGfxP++, G_CULL_BACK | G_SHADING_SMOOTH);
		gSPEndDisplayList(sphGfxP++);
	}
	
	OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
	Matrix_Push();
	Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
	Matrix_Scale(radius / 128.0f, radius / 128.0f, radius / 128.0f, MTXMODE_APPLY);
	
	gSPMatrix(
		(*gfxP)++,
		Matrix_NewMtx(gGlobalContext.state.gfxCtx, __FILE__, __LINE__),
		G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH
	);
	gSPDisplayList((*gfxP)++, pSphGfx);
	gSPPopMatrix((*gfxP)++, G_MTX_MODELVIEW);
	
	Matrix_Pop();
	CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}

static void DebugMenu_DrawHitboxList(GlobalContext* globalCtx, Gfx** gfxP, Collider** colList, s32 n) {
	for (s32 i = 0; i < n; i++) {
		Collider* col = colList[i];
		
		switch (col->shape) {
			case COLSHAPE_JNTSPH: {
				ColliderJntSph* colJntSph = (ColliderJntSph*)col;
				
				for (s32 j = 0; j < colJntSph->count; j++) {
					ColliderJntSphElement* colSphElem = &colJntSph->elements[j];
					
					DebugMenu_DrawSph(
						globalCtx,
						gfxP,
						&colSphElem->dim.worldSphere.center,
						colSphElem->dim.worldSphere.radius
					);
				}
			}
			break;
			case COLSHAPE_CYLINDER: {
				ColliderCylinder* colCyl = (ColliderCylinder*)col;
				
				DebugMenu_DrawCyl(
					globalCtx,
					gfxP,
					&colCyl->dim.pos,
					colCyl->dim.radius,
					colCyl->dim.height
				);
			}
			break;
			case COLSHAPE_TRIS: {
				ColliderTris* colTris = (ColliderTris*)col;
				
				for (s32 j = 0; j < colTris->count; j++) {
					ColliderTrisElement* colTrisElem = &colTris->elements[j];
					
					DebugMenu_DrawTri3D(
						globalCtx,
						gfxP,
						&colTrisElem->dim.vtx[0],
						&colTrisElem->dim.vtx[2],
						&colTrisElem->dim.vtx[1]
					);
				}
			}
			break;
			case COLSHAPE_QUAD: {
				ColliderQuad* colQuad = (ColliderQuad*)col;
				
				DebugMenu_DrawQuad3D(
					globalCtx,
					gfxP,
					&colQuad->dim.quad[0],
					&colQuad->dim.quad[2],
					&colQuad->dim.quad[3],
					&colQuad->dim.quad[1]
				);
			}
			break;
			case COLSHAPE_INVALID:
				break;
		}
	}
}

static void DebugMenu_DrawCollision(GlobalContext* globalCtx, Gfx** gfxP, Gfx** gfxD, CollisionHeader* colHeader) {
	if (colHeader == NULL)
		return;
	
	for (s32 i = 0; i < colHeader->numPolygons; i++) {
		CollisionPoly* colPoly = &colHeader->polyList[i];
		PolygonTypes* type = (PolygonTypes*)&colHeader->surfaceTypeList[colPoly->type];
		
		if (type->hookshot == 1)
			gDPSetPrimColor((*gfxP)++, 0, 0, 128, 128, 255, 255);
		else if (type->wallParams > SURFACE_WALL_NO_LEDGE_GRAB)
			gDPSetPrimColor((*gfxP)++, 0, 0, 192, 0, 192, 255);
		else if (type->floorParams == SURFACE_FLOOR_VOID)
			gDPSetPrimColor((*gfxP)++, 0, 0, 255, 0, 0, 255);
		else if (type->exit != 0 || type->floorParams == SURFACE_FLOOR_VOID_SMALL)
			gDPSetPrimColor((*gfxP)++, 0, 0, 0, 255, 0, 255);
		else if (type->behaviour != 0 || type->wallDamage)
			gDPSetPrimColor((*gfxP)++, 0, 0, 192, 255, 192, 255);
		else if (type->slope == 1)
			gDPSetPrimColor((*gfxP)++, 0, 0, 255, 255, 128, 255);
		else
			gDPSetPrimColor((*gfxP)++, 0, 0, 255, 255, 255, 255);
		
		Vtx vtx[3];
		for (s32 j = 0; j < 3; j++) {
			vtx[j] = gdSPDefVtxN(
				colHeader->vtxList[colPoly->vtxData[j] & 0x1FFF].x,
				colHeader->vtxList[colPoly->vtxData[j] & 0x1FFF].y,
				colHeader->vtxList[colPoly->vtxData[j] & 0x1FFF].z,
				0,
				0,
				colPoly->normal.x / 0x100,
				colPoly->normal.y / 0x100,
				colPoly->normal.z / 0x100,
				0xFF
			);
		}
		gSPVertex((*gfxP)++, gDisplayListData(gfxD, vtx), 3, 0);
		gSP1Triangle((*gfxP)++, 0, 1, 2, 0);
	}
}

static void DebugMenu_DrawQuad(GlobalContext* globalCtx, Vec2f* pos, Vec2f* scale, Color_RGBA8* color) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	
	Matrix_Translate(pos->x, pos->y, 0, MTXMODE_NEW);
	Matrix_Scale(scale->x, scale->y, 1.0f, MTXMODE_APPLY);
	
	func_800949A8(globalCtx->state.gfxCtx);
	gSPClearGeometryMode(OVERLAY_DISP++, G_CULL_BOTH | G_ZBUFFER);
	gDPSetCombineMode(OVERLAY_DISP++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);
	gDPSetPrimColor(OVERLAY_DISP++, 0, 0, color->r, color->g, color->b, color->a);
	
	gSPMatrix(OVERLAY_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD);
	gSPVertex(OVERLAY_DISP++, debugSysCtx->vtx, 4, 0);
	gSP1Quadrangle(OVERLAY_DISP++, 0, 2, 3, 1, 0);
}

// # # # # # # # # # # # # # # # # # # # #
// # PAGE FUNC                           #
// # # # # # # # # # # # # # # # # # # # #

static void DebugMenu_CollisionViewUpdate(GlobalContext* globalCtx) {
	#define POLY_GFX_BUF_SIZE 0x4000
	static Gfx sPolyBuffer[2][POLY_GFX_BUF_SIZE];
	
	DebugState* debugSysCtx = &sDebugMenuCtx;
	
	if (!debugSysCtx->colViewEnabled) {
		return;
	}
	
	OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
	
	Gfx* dlist = sPolyBuffer[globalCtx->gameplayFrames % 2];
	Gfx* pGfx = dlist;
	Gfx* dGfx = dlist + POLY_GFX_BUF_SIZE;
#undef POLY_GFX_BUF_SIZE
	
	// Setup
	gSPDisplayList(pGfx++, sPolyGfxInit);
	gSPSetGeometryMode(pGfx++, G_CULL_BACK);
	gSPMatrix(pGfx++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
	// Draw Static Collision
	DebugMenu_DrawCollision(globalCtx, &pGfx, &dGfx, globalCtx->colCtx.colHeader);
	// Draw Dynapoly Collision
	for (s32 i = 0; i < BG_ACTOR_MAX; i++) {
		if (globalCtx->colCtx.dyna.bgActorFlags[i] & 1) {
			MtxF mtx;
			BgActor* bgActor = &globalCtx->colCtx.dyna.bgActors[i];
			
			// Manually compute dyna SRT transformation
			Matrix_Push();
			SkinMatrix_SetTranslateRotateYXZScale(
				&mtx,
				bgActor->curTransform.scale.x,
				bgActor->curTransform.scale.y,
				bgActor->curTransform.scale.z,
				bgActor->curTransform.rot.x,
				bgActor->curTransform.rot.y,
				bgActor->curTransform.rot.z,
				bgActor->curTransform.pos.x,
				bgActor->curTransform.pos.y,
				bgActor->curTransform.pos.z
			);
			Matrix_Put(&mtx);
			gSPMatrix(
				pGfx++,
				Matrix_NewMtx(globalCtx->state.gfxCtx, __FILE__, __LINE__),
				G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH
			);
			
			DebugMenu_DrawCollision(globalCtx, &pGfx, &dGfx, bgActor->colHeader);
			
			gSPPopMatrix(pGfx++, G_MTX_MODELVIEW);
			Matrix_Pop();
		}
	}
	// End
	gSPEndDisplayList(pGfx++);
	
	if (pGfx > dGfx)
		Fault_AddHungupAndCrash("[DebugMenu] Collision View Gfx out of space! : " __FILE__, __LINE__);
	
	// Add dlist to POLY_OPA
	gSPDisplayList(POLY_OPA_DISP++, dlist);
	
	CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}

static void DebugMenu_HitboxViewUpdate(GlobalContext* globalCtx) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	
	if (!debugSysCtx->hitViewEnabled) {
		return;
	}
	
	OPEN_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
	
	// dlist will be on POLY_OPA buffer
	Gfx* dlist = Graph_GfxPlusOne(POLY_OPA_DISP);
	Gfx* gfx = dlist;
	
	// Setup
	gSPDisplayList(gfx++, sPolyGfxInit);
	gSPMatrix(gfx++, &gMtxClear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
	// OC
	gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, 255);
	DebugMenu_DrawHitboxList(globalCtx, &gfx, globalCtx->colChkCtx.colOC, globalCtx->colChkCtx.colOCCount);
	// AC
	gDPSetPrimColor(gfx++, 0, 0, 0, 0, 255, 255);
	DebugMenu_DrawHitboxList(globalCtx, &gfx, globalCtx->colChkCtx.colAC, globalCtx->colChkCtx.colACCount);
	// AT
	gDPSetPrimColor(gfx++, 0, 0, 255, 0, 0, 255);
	DebugMenu_DrawHitboxList(globalCtx, &gfx, globalCtx->colChkCtx.colAT, globalCtx->colChkCtx.colATCount);
	// End
	gSPEndDisplayList(gfx++);
	
	// Branch POLY_OPA past dlist
	Graph_BranchDlist(POLY_OPA_DISP, gfx);
	POLY_OPA_DISP = gfx;
	
	// Add dlist to POLY_XLU
	gSPDisplayList(POLY_XLU_DISP++, dlist);
	
	CLOSE_DISPS(globalCtx->state.gfxCtx, __FILE__, __LINE__);
}

static void DebugMenu_ObjectMemView(GlobalContext* globalCtx) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	static u32 selectedMem = 0;
	u32 selectedColor[] = {
		0x404040FF,
		0xFFFFFF00,
	};
	Color_RGBA8 memColor[] = {
		{ 240, 104, 77, 0xF8 - 0x20 },
		{ 240, 169, 77, 0xF8 - 0x20 },
		{ 191, 240, 77, 0xF8 - 0x20 },
		{ 77, 240, 153, 0xF8 - 0x20 },
		{ 77, 205, 240, 0xF8 - 0x20 },
		{ 77, 115, 240, 0xF8 - 0x20 },
		{ 137, 77, 240, 0xF8 - 0x20 },
		{ 240, 77, 175, 0xF8 - 0x20 },
	};
	
	Debug_Text(
		0x146EFFFF,
		1,
		5,
		"Object Memory View",
		0
	);
	
	debugSysCtx->vtx = Gui_AllocQuad(globalCtx, 0, -5, 1, 10, 16, 16);
	
	Vec2f pos = { -150.0f, 55.0f };
	Vec2f scale = { 300.0f, 1.10f };
	Color_RGBA8 color;
	
	scale.x += 2.0f;
	pos.x -= 1.0f;
	scale.y *= 1.2f;
	color = (Color_RGBA8) { 0x6E, 0x14, 0xFF, 0xFF };
	DebugMenu_DrawQuad(globalCtx, &pos, &scale, &color);
	
	pos.x = -150.0f;
	scale = (Vec2f) { 300.0f, 1.10f };
	color = (Color_RGBA8) { 0x30, 0x30, 0x30, 0xFF };
	DebugMenu_DrawQuad(globalCtx, &pos, &scale, &color);
	
	ObjectContext* objCtx = &globalCtx->objectCtx;
	u32 objMemSize = (u32)objCtx->spaceEnd - (u32)objCtx->spaceStart;
	
	Debug_Text(
		selectedColor[selectedMem == 0],
		1,
		6,
		"Size: %0.2f / %0.2fMB",
		(f32)((u32)objCtx->status[objCtx->num].segment - (u32)objCtx->spaceStart) / 0x100000,
		(f32)objMemSize / 0x100000
	);
	
	if (objMemSize == 0 || objCtx->num == 0)
		return;
	
	for (s32 i = 0; i <= objCtx->num; i++) {
		if (objCtx->status[i].id <= 0)
			continue;
		
		u32 objSize = gObjectTable[objCtx->status[i].id].vromEnd - gObjectTable[objCtx->status[i].id].vromStart;
		
		pos.x = (f32)((u32)objCtx->status[i].segment - (u32)objCtx->spaceStart) / objMemSize;
		pos.x *= 300.0f;
		scale.x = (f32)objSize / objMemSize;
		
		color = memColor[i % 8];
		color.a = 0xE0;
		
		pos.x -= 150.0f;
		scale.x *= 300.0f;
		DebugMenu_DrawQuad(globalCtx, &pos, &scale, &color);
	}
}

static void DebugMenu_ZeldaArenaMemView(GlobalContext* globalCtx) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	static u32 selectedMem = 0;
	u32 selectedColor[] = {
		0x404040FF,
		0xFFFFFF00,
	};
	Color_RGBA8 memColor[] = {
		{ 240, 104, 77, 0xF8 - 0x20 },
		{ 240, 169, 77, 0xF8 - 0x20 },
		{ 191, 240, 77, 0xF8 - 0x20 },
		{ 77, 240, 153, 0xF8 - 0x20 },
		{ 77, 205, 240, 0xF8 - 0x20 },
		{ 77, 115, 240, 0xF8 - 0x20 },
		{ 137, 77, 240, 0xF8 - 0x20 },
		{ 240, 77, 175, 0xF8 - 0x20 },
	};
	
	Debug_Text(
		0x146EFFFF,
		1,
		5,
		"ZeldaArena Memory View",
		0
	);
	
	debugSysCtx->vtx = Gui_AllocQuad(globalCtx, 0, -5, 1, 10, 16, 16);
	
	Vec2f pos = { -150.0f, 55.0f };
	Vec2f scale = { 300.0f, 1.10f };
	Color_RGBA8 color;
	
	scale.x += 2.0f;
	pos.x -= 1.0f;
	scale.y *= 1.2f;
	color = (Color_RGBA8) { 0x6E, 0x14, 0xFF, 0xFF };
	DebugMenu_DrawQuad(globalCtx, &pos, &scale, &color);
	
	pos.x = -150.0f;
	scale = (Vec2f) { 300.0f, 1.10f };
	color = (Color_RGBA8) { 0x30, 0x30, 0x30, 0xFF };
	DebugMenu_DrawQuad(globalCtx, &pos, &scale, &color);
	
	u32 outMaxFree;
	u32 outFree;
	u32 outAlloc;
	
	ZeldaArena_GetSizes(&outMaxFree, &outFree, &outAlloc);
	
	Debug_Text(
		selectedColor[selectedMem == 0],
		1,
		6,
		"Size: %0.2f / %0.2fMB",
		(f32)((outAlloc + outFree) - outFree) / 0x100000,
		(f32)(outAlloc + outFree) / 0x100000
	);
	
	ArenaNode* arena = sZeldaArena.head;
	u32 i = 0;
	
	if (!arena)
		return;
	
	while (arena) {
		f32 arenaSize = arena->size;
		f32 arenaStart = (u32)arena - (u32)sZeldaArena.head;
		
		pos.x = arenaStart / (outAlloc + outFree);
		pos.x *= 300.0f;
		scale.x = arenaSize / (outAlloc + outFree);
		
		color = memColor[i % 8];
		if (arena->isFree) {
			color = (Color_RGBA8) { 0x40, 0x40, 0x40, 0xF8 - 0x20 };
		}
		color.a = 0xE0;
		
		pos.x -= 150.0f;
		scale.x *= 300.0f;
		DebugMenu_DrawQuad(globalCtx, &pos, &scale, &color);
		i++;
		
		arena = arena->next;
	}
}

static void DebugMenu_HitboxView(GlobalContext* globalCtx) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	
	debugSysCtx->hitViewEnabled ^= 1;
	debugSysCtx->page = DEBUGSYS_PAGE_MAIN;
}

static void DebugMenu_CollisionView(GlobalContext* globalCtx) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	
	debugSysCtx->colViewEnabled ^= 1;
	debugSysCtx->page = DEBUGSYS_PAGE_MAIN;
}

static void DebugMenu_CineMode(GlobalContext* globalCtx) {
	DebugState* debugSysCtx = &sDebugMenuCtx;
	
	debugSysCtx->cineModeEnabled ^= 1;
	debugSysCtx->page = DEBUGSYS_PAGE_MAIN;
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static DebugPageInfo sDebugPageInfo[] = {
	{
		.func = NULL,
		.name = "\0",
		.playerFreeze = true,
	},
	{
		.func = DebugMenu_ObjectMemView,
		.name = "Object Memory View",
		.playerFreeze = false,
	},
	{
		.func = DebugMenu_ZeldaArenaMemView,
		.name = "Zelda Arena Memory View",
		.playerFreeze = false,
	},
	{
		.func = DebugMenu_CollisionView,
		.name = "Toggle Collision Viewer",
		.playerFreeze = false,
	},
	{
		.func = DebugMenu_HitboxView,
		.name = "Toggle Hitbox Viewer",
		.playerFreeze = false,
	},
	{
		.func = DebugMenu_CineMode,
		.name = "Toggle Cine Mode",
		.playerFreeze = false,
	},
};

static s16 sInterfacePrevAlpha[16];
static s16 sInterfaceFlag;
static s16 sInterfaceForceHide;

static void DebugMenu_Interface_Hide(InterfaceContext* interface) {
	if (sInterfaceFlag == 0) {
		sInterfacePrevAlpha[0] = interface->aAlpha;
		sInterfacePrevAlpha[1] = interface->bAlpha;
		sInterfacePrevAlpha[2] = interface->healthAlpha;
		sInterfacePrevAlpha[3] = interface->magicAlpha;
		sInterfacePrevAlpha[4] = interface->minimapAlpha;
		sInterfacePrevAlpha[5] = interface->startAlpha;
		sInterfacePrevAlpha[6] = interface->cDownAlpha;
		sInterfacePrevAlpha[7] = interface->cLeftAlpha;
		sInterfacePrevAlpha[8] = interface->cRightAlpha;
		sInterfaceFlag = 1;
	}
	
	interface->aAlpha = 0;
	interface->bAlpha = 0;
	interface->healthAlpha = 0;
	interface->magicAlpha = 0;
	interface->minimapAlpha = 0;
	interface->startAlpha = 0;
	interface->cDownAlpha = 0;
	interface->cLeftAlpha = 0;
	interface->cRightAlpha = 0;
}

static void DebugMenu_Interface_Show(InterfaceContext* interface) {
	if (sInterfaceForceHide)
		return;
	
	if (sInterfaceFlag == 1) {
		interface->aAlpha = sInterfacePrevAlpha[0];
		interface->bAlpha = sInterfacePrevAlpha[1];
		interface->healthAlpha = sInterfacePrevAlpha[2];
		interface->magicAlpha = sInterfacePrevAlpha[3];
		interface->minimapAlpha = sInterfacePrevAlpha[4];
		interface->startAlpha = sInterfacePrevAlpha[5];
		interface->cDownAlpha = sInterfacePrevAlpha[6];
		interface->cLeftAlpha = sInterfacePrevAlpha[7];
		interface->cRightAlpha = sInterfacePrevAlpha[8];
		sInterfaceFlag = 0;
	}
}

static void DebugMenu_MenuUpdate(GlobalContext* globalCtx) {
	Player* p = GET_PLAYER(globalCtx);
	DebugState* debugSysCtx = &sDebugMenuCtx;
	u8 holdR = CHK_ALL(cur, BTN_R);
	u32 infoMax = ARRAY_COUNT(sDebugPageInfo);
	
	sInterfaceForceHide = false;
	
	if (globalCtx->pauseCtx.state != 0)
		return;
	
	if (debugSysCtx->cineModeEnabled) {
		ShrinkWindow_SetVal(0x20);
		
		DebugMenu_Interface_Hide(&globalCtx->interfaceCtx);
		sInterfaceForceHide = true;
	}
	
	/* Flip ON flag */
	if (CHK_ALL(press, BTN_R | BTN_L | BTN_B)) {
		debugSysCtx->state.on ^= 1;
		if (p->stateFlags1 & (1 << 29)) {
			p->stateFlags1 &= ~(1 << 29);
		}
		
		if (debugSysCtx->state.on)
			Audio_PlaySys(NA_SE_SY_FSEL_DECIDE_S);
		
		else {
			Audio_PlaySys(NA_SE_SY_FSEL_CLOSE);
			DebugMenu_Interface_Show(&globalCtx->interfaceCtx);
		}
		
		return;
	}
	
	if (debugSysCtx->state.on == 0) {
		return;
	}
	
	if (holdR && CHK_ALL(press, BTN_B)) {
		DebugMenu_Interface_Show(&globalCtx->interfaceCtx);
		if (debugSysCtx->page == DEBUGSYS_PAGE_MAIN) {
			debugSysCtx->state.on = 0;
			p->stateFlags1 &= ~(1 << 29);
			Audio_PlaySys(NA_SE_SY_FSEL_CLOSE);
			
			return;
		}
		debugSysCtx->page = DEBUGSYS_PAGE_MAIN;
		Audio_PlaySys(NA_SE_SY_FSEL_CLOSE);
	}
	
	/* Freeze player based on PAGE info */
	if (sDebugPageInfo[debugSysCtx->page].playerFreeze == 1) {
		ShrinkWindow_SetVal(0x20);
		p->stateFlags1 |= (1 << 29);
		DebugMenu_Interface_Hide(&globalCtx->interfaceCtx);
	} else {
		p->stateFlags1 &= ~(1 << 29);
		DebugMenu_Interface_Show(&globalCtx->interfaceCtx);
	}
	
	if (sDebugPageInfo[debugSysCtx->page].func) {
		sDebugPageInfo[debugSysCtx->page].func(globalCtx);
		
		return;
	}
	
	/* MAIN MENU */
	
	u32 oldSetPage = debugSysCtx->setPage;
	
	if (holdR && CHK_ALL(press, BTN_A)) {
		debugSysCtx->page = debugSysCtx->setPage;
		Audio_PlaySys(NA_SE_SY_FSEL_DECIDE_S);
	}
	if (holdR && CHK_ALL(press, BTN_DUP)) {
		debugSysCtx->setPage = CLAMP((debugSysCtx->setPage - 1), 1, infoMax - 1);
	}
	if (holdR && CHK_ALL(press, BTN_DDOWN)) {
		debugSysCtx->setPage = CLAMP((debugSysCtx->setPage + 1), 1, infoMax - 1);
	}
	debugSysCtx->setPage = CLAMP(debugSysCtx->setPage, 1, infoMax - 1);
	
	if (oldSetPage != debugSysCtx->setPage) {
		Audio_PlaySys(NA_SE_IT_SWORD_IMPACT);
	}
	
	Debug_Text(0x146EFFFF, 1, 5, "Debug Menu");
	Debug_Text(0x404040FF, 1, 5, "               (HoldR, A + DPAD)");
	
	for (s32 i = 1; i < infoMax; i++) {
		u32 color = 0xede1beFF;
		
		if (i == debugSysCtx->setPage) {
			color = 0xff6314FF;
		}
		
		Debug_Text(
			color,
			1,
			5 + i,
			"%s",
			sDebugPageInfo[i].name
		);
	}
}

void DebugMenu_Update(GlobalContext* globalCtx) {
	DebugMenu_MenuUpdate(globalCtx);
	if (gSaveContext.gameMode == 0 && globalCtx->pauseCtx.mode == 0) {
		DebugMenu_CollisionViewUpdate(globalCtx);
		DebugMenu_HitboxViewUpdate(globalCtx);
	}
}

#endif