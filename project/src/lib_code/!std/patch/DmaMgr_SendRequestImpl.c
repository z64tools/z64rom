#define NO_Z64ROM_EXT_TABLES
#include <uLib.h>
#include "boot/z_std_dma.h"

/*
   z64ram = 0x80001664
   z64rom = 0x002264
   z64next = 0x800017C0
 */

s32 DmaMgr_SendRequestImpl(DmaRequest* req, void* ram, uintptr_t vrom, u32 size, u32 unk, OSMesgQueue* queue, OSMesg msg) {
    static s32 sDmaMgrQueueFullLogged = 0;
    
    req->vromAddr = vrom;
    req->dramAddr = (void*)ram;
    req->size = size;
    req->unk_14 = 0;
    req->notifyQueue = queue;
    req->notifyMsg = msg;
    
    if (
        (1 && (ram == 0)) || (osMemSize < OS_K0_TO_PHYSICAL(ram) + size) || (vrom & 1) || (vrom > 0x4000000) || (size == 0) ||
        (size & 1)
    ) {
        DmaMgr_Error(req, NULL, "Illegal", "Illegal");
    }
    
    if (1 && (sDmaMgrQueueFullLogged == 0) && MQ_IS_FULL(&sDmaMgrMsgQueue)) {
        sDmaMgrQueueFullLogged++;
    }
    
    osSendMesg(&sDmaMgrMsgQueue, (OSMesg)req, OS_MESG_BLOCK);
    
    return 0;
}
