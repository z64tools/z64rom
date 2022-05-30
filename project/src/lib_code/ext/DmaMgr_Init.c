#define __NO_EXT_MACROS__
#include <uLib.h>

/*
   z64ram = 0x8000183C
   z64rom = 0x00243C
 */

#define THREAD_PRI_DMAMGR 16

#define THREAD_ID_IDLE     1
#define THREAD_ID_FAULT    2
#define THREAD_ID_MAIN     3
#define THREAD_ID_GRAPH    4
#define THREAD_ID_SCHED    5
#define THREAD_ID_PADMGR   7
#define THREAD_ID_AUDIOMGR 10
#define THREAD_ID_DMAMGR   18
#define THREAD_ID_IRQMGR   19

#define STACK(stack, size) \
	u64 stack[ALIGN8(size) / sizeof(u64)]

#define STACK_TOP(stack) \
	((u8*)(stack) + sizeof(stack))

void DmaMgr_Init(void) {
	DmaEntry* iter;
	OSMesg* sDmaMgrMsgBuf = (void*)0x800139A8; // [32]
	
	DmaMgr_DmaRomToRam(
		0x012F70,
		(u32)gDmaDataTable,
		sizeof(DmaEntry) * 4
	);
	
	DmaMgr_DmaRomToRam(
		gDmaDataTable[3].romStart ? gDmaDataTable[3].romStart : gDmaDataTable[3].vromStart,
		0x80700000,
		gDmaDataTable[3].vromEnd - gDmaDataTable[3].vromStart
	);
	
	DmaMgr_DmaRomToRam(
		gDmaDataTable[2].vromStart,
		(u32)__ext_gDmaDataTable,
		gDmaDataTable[2].vromEnd - gDmaDataTable[2].vromStart
	);
	
	sDmaMgrIsRomCompressed = false;
	iter = gDmaDataTable;
	
	while (iter->vromEnd != 0) {
		if (iter->romEnd != 0) {
			sDmaMgrIsRomCompressed = true;
		}
		
		iter++;
	}
	
	osCreateMesgQueue(&sDmaMgrMsgQueue, sDmaMgrMsgBuf, 32);
	StackCheck_Init(&sDmaMgrStackInfo, sDmaMgrStack, STACK_TOP(sDmaMgrStack), 0, 0x100, "dmamgr");
	osCreateThread(
		&sDmaMgrThread,
		THREAD_ID_DMAMGR,
		DmaMgr_ThreadEntry,
		NULL,
		STACK_TOP(sDmaMgrStack),
		THREAD_PRI_DMAMGR
	);
	osStartThread(&sDmaMgrThread);
}