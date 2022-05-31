#include <uLib.h>

/*
   z64ram = 0x800013FC
   z64rom = 0x001FFC
 */

void DmaMgr_ProcessMsg(DmaRequest* req) {
	u32 vrom = req->vromAddr;
	void* ram = req->dramAddr;
	u32 size = req->size;
	u32 romStart;
	u32 romSize;
	u8 found = false;
	DmaEntry* iter;
	
	iter = gDmaDataTable;
	
	Debug_DmaLog(req);
	while (iter->vromEnd) {
		if (vrom >= iter->vromStart && vrom < iter->vromEnd) {
			if (iter->romEnd == 0) {
				DmaMgr_DmaRomToRam(iter->romStart + (vrom - iter->vromStart), (u32)ram, size);
				found = true;
			} else {
				romStart = iter->romStart;
				romSize = iter->romEnd - iter->romStart;
				
				osSetThreadPri(NULL, Z_PRIORITY_MAIN);
				Yaz0_Decompress(romStart, ram, romSize);
				osSetThreadPri(NULL, Z_PRIORITY_DMAMGR);
				found = true;
			}
			
			if (found) break;
		}
		iter++;
	}
	
	if (!found) {
		if (sDmaMgrIsRomCompressed) {
			osLibPrintf("NoData");
			
			return;
		}
		DmaMgr_DmaRomToRam(vrom, (u32)ram, size);
	}
	osLibPrintf("OK");
}