#include <oot_mq_debug/z64hdr.h>

/*
   z64ram = 0x800C6E20
   z64rom = 0xB3DFC0
 */

extern PreNmiBuff* gAppNmiBufferPtr;
extern SchedContext gSchedContext;
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
extern OSMesgQueue sSiIntMsgQ;
extern OSMesg sSiIntMsgBuf[1];
extern u32 gSystemHeapSize;

void Main(void* arg) {
	IrqMgrClient irqClient;
	OSMesgQueue irqMgrMsgQ;
	OSMesg irqMgrMsgBuf[60];
	u32 sysHeap;
	u32 fb;
	s16* msg;
	
	gScreenWidth = SCREEN_WIDTH;
	gScreenHeight = SCREEN_HEIGHT;
	gAppNmiBufferPtr = (PreNmiBuff*)osAppNmiBuffer;
	PreNmiBuff_Init(gAppNmiBufferPtr);
	Fault_Init();
	SysCfb_Init(0);
	sysHeap = (u32)gAudioHeap; // gSystemHeap
	fb = SysCfb_GetFbPtr(0);
	gSystemHeapSize = (0x806C0000 - sysHeap); // (fb - sysHeap)
	SystemHeap_Init((void*)sysHeap, gSystemHeapSize);
	func_800636C0();
	
	R_ENABLE_ARENA_DBG = 0;
	
	osCreateMesgQueue(&sSiIntMsgQ, sSiIntMsgBuf, 1);
	osSetEventMesg(5, &sSiIntMsgQ, 0);
	
	Main_LogSystemHeap();
	
	osCreateMesgQueue(&irqMgrMsgQ, irqMgrMsgBuf, 0x3c);
	StackCheck_Init(&sIrqMgrStackInfo, sIrqMgrStack, sIrqMgrStack + sizeof(sIrqMgrStack), 0, 0x100, "irqmgr");
	IrqMgr_Init(&gIrqMgr, &sGraphStackInfo, Z_PRIORITY_IRQMGR, 1);
	
	StackCheck_Init(&sSchedStackInfo, sSchedStack, sSchedStack + sizeof(sSchedStack), 0, 0x100, "sched");
	Sched_Init(&gSchedContext, &sAudioStack, Z_PRIORITY_SCHED, D_80013960, 1, &gIrqMgr);
	
	IrqMgr_AddClient(&gIrqMgr, &irqClient, &irqMgrMsgQ);
	
	StackCheck_Init(&sAudioStackInfo, sAudioStack, sAudioStack + sizeof(sAudioStack), 0, 0x100, "audio");
	AudioMgr_Init(&gAudioMgr, sAudioStack + sizeof(sAudioStack), Z_PRIORITY_AUDIOMGR, 0xa, &gSchedContext, &gIrqMgr);
	
	StackCheck_Init(&sPadMgrStackInfo, sPadMgrStack, sPadMgrStack + sizeof(sPadMgrStack), 0, 0x100, "padmgr");
	PadMgr_Init(&gPadMgr, &sSiIntMsgQ, &gIrqMgr, 7, Z_PRIORITY_PADMGR, &sIrqMgrStack);
	
	AudioMgr_Unlock(&gAudioMgr);
	
	StackCheck_Init(&sGraphStackInfo, sGraphStack, sGraphStack + sizeof(sGraphStack), 0, 0x100, "graph");
	osCreateThread(&sGraphThread, 4, Graph_ThreadEntry, arg, sGraphStack + sizeof(sGraphStack), Z_PRIORITY_GRAPH);
	osStartThread(&sGraphThread);
	osSetThreadPri(0, Z_PRIORITY_SCHED);
	
	while (true) {
		msg = NULL;
		osRecvMesg(&irqMgrMsgQ, (OSMesg) & msg, OS_MESG_BLOCK);
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