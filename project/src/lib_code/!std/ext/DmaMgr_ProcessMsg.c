#include <uLib.h>
#include "boot/z_std_dma.h"

/*
   z64ram = 0x800013FC
   z64rom = 0x001FFC
   z64next = 0x800015CC
 */

void DmaMgr_ProcessMsg(DmaRequest* req) {
    u32 vrom = req->vromAddr;
    void* ram = req->dramAddr;
    u32 size = req->size;
    u32 romStart;
    u32 romSize;
    u8 found = false;
    DmaEntry* iter;
    
    if (osMemSize > 0x400000U) {
        iter = gDmaDataTable;
        Debug_DmaLog(req);
    } else {
        iter = (void*)0x80016DA0;
    }
    
    while (iter->vromEnd) {
        if (vrom >= iter->vromStart && vrom < iter->vromEnd) {
            if (iter->romEnd == 0) {
                DmaMgr_DmaRomToRam(iter->romStart + (vrom - iter->vromStart), ram, size);
                found = true;
            } else {
                romStart = iter->romStart;
                romSize = iter->romEnd - iter->romStart;
                
                osSetThreadPri(NULL, THREAD_PRI_DMAMGR_LOW);
                Yaz0_Decompress(romStart, ram, romSize);
                osSetThreadPri(NULL, THREAD_PRI_DMAMGR);
                found = true;
            }
            
            if (found) break;
        }
        iter++;
    }
    
    if (!found) {
        if (sDmaMgrIsRomCompressed) {
            osSyncPrintf("NoData");
            
            return;
        }
        DmaMgr_DmaRomToRam(vrom, ram, size);
    }
}
