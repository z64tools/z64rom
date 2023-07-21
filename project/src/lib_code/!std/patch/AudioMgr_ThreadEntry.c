#include <uLib.h>

/*
   z64ram = 0x800C3E70
   z64rom = 0xB3B010
   z64next = 0x800C3FC4
 */

void AudioMgr_ThreadEntry(void* arg0) {
    AudioMgr* audioMgr = (AudioMgr*)arg0;
    IrqMgrClient irqClient;
    s16* msg = NULL;
    
    if (osMemSize > 0x400000U)
        Audio_Init();
    else
        AudioLoad_Init((void*)0x801D89E0, 0x37F00);
    
    AudioLoad_SetDmaHandler(DmaMgr_DmaHandler);
    Audio_InitSound();
    osSendMesg(&audioMgr->lockQueue, NULL, OS_MESG_BLOCK);
    IrqMgr_AddClient(audioMgr->irqMgr, &irqClient, &audioMgr->interruptQueue);
    
    while (true) {
        osRecvMesg(&audioMgr->interruptQueue, (OSMesg*)&msg, OS_MESG_BLOCK);
        switch (*msg) {
            case OS_SC_RETRACE_MSG:
                AudioMgr_HandleRetrace(audioMgr);
                while (!MQ_IS_EMPTY(&audioMgr->interruptQueue)) {
                    osRecvMesg(&audioMgr->interruptQueue, (OSMesg*)&msg, OS_MESG_BLOCK);
                    switch (*msg) {
                        case OS_SC_RETRACE_MSG:
                            break;
                        case OS_SC_PRE_NMI_MSG:
                            AudioMgr_HandlePreNMI(audioMgr);
                            break;
                    }
                }
                break;
            case OS_SC_PRE_NMI_MSG:
                AudioMgr_HandlePreNMI(audioMgr);
                break;
        }
    }
}
