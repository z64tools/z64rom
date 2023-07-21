#define NO_Z64ROM_EXT_TABLES
#include <uLib.h>
#include "boot/z_std_dma.h"

/*
   z64ram = 0x8000183C
   z64rom = 0x00243C
   z64next = 0x80001A5C
 */

void DmaMgr_Init(void) {
    DmaEntry* iter;
    OSMesg* sDmaMgrMsgBuf = (void*)0x800139A8; // [32]
    
    if (osMemSize > 0x400000U) {
        DmaMgr_DmaRomToRam(0x012F70, gDmaDataTable, sizeof(DmaEntry) * 4);
        
        if (gDmaDataTable[3].romEnd) {
            u32 romStart = gDmaDataTable[3].romStart;
            u32 romSize = gDmaDataTable[3].romEnd - gDmaDataTable[3].romStart;
            
            osSetThreadPri(NULL, THREAD_PRI_DMAMGR_LOW);
            Yaz0_Decompress(romStart, (void*)0x80700000, romSize);
            osSetThreadPri(NULL, THREAD_PRI_DMAMGR);
        } else {
            DmaMgr_DmaRomToRam(gDmaDataTable[3].vromStart, (void*)0x80700000,
                gDmaDataTable[3].vromEnd - gDmaDataTable[3].vromStart);
        }
        
        DmaMgr_DmaRomToRam(gDmaDataTable[2].vromStart, __ext_gDmaDataTable,
            gDmaDataTable[2].vromEnd - gDmaDataTable[2].vromStart);
    } else {
        DmaMgr_DmaRomToRam(0x012F70, gDmaDataTable,
            sizeof(DmaEntry) * 0x60C);
        gDmaDataTable[0x60B].vromEnd = 0;
    }
    
    sDmaMgrIsRomCompressed = false;
    iter = gDmaDataTable;
    
    for (; iter->vromEnd != 0; iter++)
        if (iter->romEnd != 0)
            sDmaMgrIsRomCompressed = true;
    
    osCreateMesgQueue(&sDmaMgrMsgQueue, sDmaMgrMsgBuf, 32);
    StackCheck_Init(&sDmaMgrStackInfo, sDmaMgrStack, STACK_TOP(sDmaMgrStack),
        0, 0x100, "");
    osCreateThread(&sDmaMgrThread, THREAD_ID_DMAMGR, DmaMgr_ThreadEntry,
        NULL, STACK_TOP(sDmaMgrStack), THREAD_PRI_DMAMGR);
    osStartThread(&sDmaMgrThread);
}
