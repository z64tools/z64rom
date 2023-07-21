#include <uLib.h>

/*
   z64ram = 0x800C6E20
   z64rom = 0xB3DFC0
   z64next = 0x800c7250
 */

extern PreNmiBuff* gAppNmiBufferPtr;
extern Scheduler gScheduler;
extern PadMgr gPadMgr;
extern IrqMgr gIrqMgr;
extern OSThread sGraphThread;
extern u8 sGraphStack[0x1800];
extern u8 sSchedStack[0x600];
extern u8 sAudioStack[0x800];
extern u8 sPadMgrStack[0x500];
extern u8 sIrqMgrStack[0x500];
extern StackEntry sGraphStackInfo;
extern StackEntry sSchedStackInfo;
extern StackEntry sAudioStackInfo;
extern StackEntry sPadMgrStackInfo;
extern StackEntry sIrqMgrStackInfo;
extern AudioMgr gAudioMgr;
extern OSMesgQueue sSerialEventQueue;
extern OSMesg sSerialMsgBuf[1];
extern u32 gSystemHeapSize;

asm ("osAppNMIBuffer = 0x8000031C");

void Main(void* arg) {
    IrqMgrClient irqClient;
    OSMesgQueue irqMgrMsgQueue;
    OSMesg irqMgrMsgBuf[60];
    u32 sysHeap;
    s16* msg;
    
    gScreenWidth = SCREEN_WIDTH;
    gScreenHeight = SCREEN_HEIGHT;
    gAppNmiBufferPtr = (PreNmiBuff*)osAppNMIBuffer;
    PreNmiBuff_Init(gAppNmiBufferPtr);
    Fault_Init();
    SysCfb_Init(0);
    
    if (osMemSize > 0x400000U) {
        sysHeap = (u32)gAudioHeap; // gSystemHeap
        gSystemHeapSize = (0x806C0000 - sysHeap); // (fb - sysHeap)
    } else {
        // Repurposed Function
        GameState_DrawInputDisplay(0, 0);
        sysHeap = 0x802109E0; // Original Heap
        gSystemHeapSize = (u32)SysCfb_GetFbPtr(0) - sysHeap;
    }
    
    SystemHeap_Init((void*)sysHeap, gSystemHeapSize);
    func_800636C0();
    
    R_ENABLE_ARENA_DBG = 0;
    
    osCreateMesgQueue(&sSerialEventQueue, sSerialMsgBuf, 1);
    osSetEventMesg(5, &sSerialEventQueue, 0);
    
    Main_LogSystemHeap();
    
    osCreateMesgQueue(&irqMgrMsgQueue, irqMgrMsgBuf, ARRAY_COUNT(irqMgrMsgBuf));
    StackCheck_Init(&sIrqMgrStackInfo, sIrqMgrStack, STACK_TOP(sIrqMgrStack), 0, 0x100, "irqmgr");
    IrqMgr_Init(&gIrqMgr, STACK_TOP(sIrqMgrStack), THREAD_PRI_IRQMGR, 1);
    
    StackCheck_Init(&sSchedStackInfo, sSchedStack, STACK_TOP(sSchedStack), 0, 0x100, "sched");
    Sched_Init(&gScheduler, STACK_TOP(sSchedStack), THREAD_PRI_SCHED, D_80013960, 1, &gIrqMgr);
    
    IrqMgr_AddClient(&gIrqMgr, &irqClient, &irqMgrMsgQueue);
    
    StackCheck_Init(&sAudioStackInfo, sAudioStack, STACK_TOP(sAudioStack), 0, 0x100, "audio");
    AudioMgr_Init(&gAudioMgr, STACK_TOP(sAudioStack), THREAD_PRI_AUDIOMGR, THREAD_ID_AUDIOMGR, &gScheduler, &gIrqMgr);
    
    StackCheck_Init(&sPadMgrStackInfo, sPadMgrStack, STACK_TOP(sPadMgrStack), 0, 0x100, "padmgr");
    PadMgr_Init(&gPadMgr, &sSerialEventQueue, &gIrqMgr, THREAD_ID_PADMGR, THREAD_PRI_PADMGR, STACK_TOP(sPadMgrStack));
    
    AudioMgr_Unlock(&gAudioMgr);
    
    StackCheck_Init(&sGraphStackInfo, sGraphStack, STACK_TOP(sGraphStack), 0, 0x100, "graph");
    osCreateThread(&sGraphThread, THREAD_ID_GRAPH, Graph_ThreadEntry, arg, STACK_TOP(sGraphStack), THREAD_PRI_GRAPH);
    osStartThread(&sGraphThread);
    osSetThreadPri(NULL, THREAD_PRI_MAIN);
    
    while (true) {
        msg = NULL;
        osRecvMesg(&irqMgrMsgQueue, (OSMesg*)&msg, OS_MESG_BLOCK);
        if (msg == NULL) {
            break;
        }
        if (*msg == OS_SC_PRE_NMI_MSG) {
            PreNmiBuff_SetReset(gAppNmiBufferPtr);
        }
    }
    
    osDestroyThread(&sGraphThread);
    RcpUtils_Reset();
}
