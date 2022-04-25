#include <ULib.h>

/*
   z64ram = 0x800005A0
   z64rom = 0x0011A0
 */

void Main_ThreadEntry(void* arg) {
	OSTime time;
	
	DmaMgr_Init();
	time = osGetTime();
	DmaMgr_SendRequest1(_codeSegmentStart, gDmaDataTable[28].vromStart, gDmaDataTable[28].vromEnd - gDmaDataTable[28].vromStart, "Main_ThreadEntry", __LINE__);
	time -= osGetTime();
	MemSet(_codeSegmentBssStart, 0, _codeSegmentBssEnd - _codeSegmentBssStart);
	Main(arg);
}