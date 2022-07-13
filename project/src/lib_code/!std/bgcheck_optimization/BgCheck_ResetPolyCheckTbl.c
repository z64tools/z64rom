#include <uLib.h>
#include "code/z_bgcheck.h"

/*
   z64ram = 0x80041978
   z64rom = 0xAB8B18
   z64next = 0x800419B0
 */

void BgCheck_ResetPolyCheckTbl(SSNodeList* nodeList, s32 numPolys) {
	
	bzero(nodeList->polyCheckTbl, numPolys);
}