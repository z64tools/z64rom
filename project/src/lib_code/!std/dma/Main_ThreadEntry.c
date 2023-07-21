#include <uLib.h>
#include "code/code_80069420.h"

/*
   z64ram = 0x800005A0
   z64rom = 0x0011A0
   z64next = 0x80000694
 */

asm ("_codeSegmentStart = 0x8001ce60");
asm ("_codeSegmentBssEnd = 0x801759c0");
asm ("_codeSegmentBssStart = 0x80157d90");

void Main_ThreadEntry(void* arg) {
    DmaEntry* dma;
    
    if (osMemSize <= 0x400000U)
        dma = (void*)0x80016DA0;
    else
        dma = gDmaDataTable;
    
    DmaMgr_Init();
    DmaMgr_SendRequest1(_codeSegmentStart, dma[28].vromStart, dma[28].vromEnd - dma[28].vromStart, "Main_ThreadEntry", __LINE__);
    MemSet(_codeSegmentBssStart, 0, _codeSegmentBssEnd - _codeSegmentBssStart);
    Main(arg);
}
