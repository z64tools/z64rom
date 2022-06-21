#include <uLib.h>
#include <code/z_bgcheck.h>

/*
   z64ram = 0x8003C078
   z64rom = 0xAB3218
   z64next = 0x8002F298
 */

void BgCheck_Allocate(CollisionContext* colCtx, PlayState* play, CollisionHeader* colHeader) {
	u32 tblMax;
	u32 memSize;
	
	colCtx->colHeader = colHeader;
	
	colCtx->memSize = 0xF000 * 8;
	colCtx->dyna.polyNodesMax = 1000 * 8;
	colCtx->dyna.polyListMax = 512 * 8;
	colCtx->dyna.vtxListMax = 512 * 8;
	colCtx->subdivAmount.x = 16;
	colCtx->subdivAmount.y = 4;
	colCtx->subdivAmount.z = 16;
	
	colCtx->lookupTbl = THA_AllocEndAlign(
		&play->state.tha,
		colCtx->subdivAmount.x * sizeof(StaticLookup) * colCtx->subdivAmount.y * colCtx->subdivAmount.z,
		~1
	);
	if (colCtx->lookupTbl == NULL) {
		LogUtils_HungupThread(0, 0);
	}
	colCtx->minBounds.x = colCtx->colHeader->minBounds.x;
	colCtx->minBounds.y = colCtx->colHeader->minBounds.y;
	colCtx->minBounds.z = colCtx->colHeader->minBounds.z;
	colCtx->maxBounds.x = colCtx->colHeader->maxBounds.x;
	colCtx->maxBounds.y = colCtx->colHeader->maxBounds.y;
	colCtx->maxBounds.z = colCtx->colHeader->maxBounds.z;
	BgCheck_SetSubdivisionDimension(
		colCtx->minBounds.x,
		colCtx->subdivAmount.x,
		&colCtx->maxBounds.x,
		&colCtx->subdivLength.x,
		&colCtx->subdivLengthInv.x
	);
	BgCheck_SetSubdivisionDimension(
		colCtx->minBounds.y,
		colCtx->subdivAmount.y,
		&colCtx->maxBounds.y,
		&colCtx->subdivLength.y,
		&colCtx->subdivLengthInv.y
	);
	BgCheck_SetSubdivisionDimension(
		colCtx->minBounds.z,
		colCtx->subdivAmount.z,
		&colCtx->maxBounds.z,
		&colCtx->subdivLength.z,
		&colCtx->subdivLengthInv.z
	);
	memSize = colCtx->subdivAmount.x * sizeof(StaticLookup) * colCtx->subdivAmount.y * colCtx->subdivAmount.z +
		colCtx->colHeader->numPolygons * sizeof(u8) + colCtx->dyna.polyNodesMax * sizeof(SSNode) +
		colCtx->dyna.polyListMax * sizeof(CollisionPoly) + colCtx->dyna.vtxListMax * sizeof(Vec3s) +
		sizeof(CollisionContext);
	
	if (colCtx->memSize < memSize) {
		LogUtils_HungupThread(0, 0);
	}
	tblMax = (colCtx->memSize - memSize) / sizeof(SSNode);
	
	SSNodeList_Initialize(&colCtx->polyNodes);
	SSNodeList_Alloc(play, &colCtx->polyNodes, tblMax, colCtx->colHeader->numPolygons);
	
	BgCheck_InitializeStaticLookup(colCtx, play, colCtx->lookupTbl);
	
	DynaPoly_Init(play, &colCtx->dyna);
	DynaPoly_Alloc(play, &colCtx->dyna);
}