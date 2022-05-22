#include <ULib.h>

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
	u32 id = 0;
	
	iter = gDmaDataTable;
	
	while (iter->vromEnd) {
		if (vrom >= iter->vromStart && vrom < iter->vromEnd) {
			if (iter->romEnd == 0 && vrom + req->size <= iter->vromEnd) {
				DmaMgr_DmaRomToRam(iter->romStart + (vrom - iter->vromStart), (u32)ram, size);
				found = true;
			} else if (vrom == iter->vromStart) {
				romStart = iter->romStart;
				romSize = iter->vromEnd - iter->romStart;
				
				osSetThreadPri(NULL, Z_PRIORITY_MAIN);
				Yaz0_Decompress(romStart, ram, romSize);
				osSetThreadPri(NULL, Z_PRIORITY_DMAMGR);
				found = true;
			}
			
			if (found) break;
		}
		iter++;
		id++;
	}
	
	if (!found) {
		DmaMgr_DmaRomToRam(vrom, (u32)ram, size);
	}
	
	uLib_DmaLog(req);
}