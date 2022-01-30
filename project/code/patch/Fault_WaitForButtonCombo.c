#include <oot_mq_debug/z64hdr.h>

/**
 * Do not require button combo for debugger crash
 * screen.
 */

void Fault_WaitForButtonCombo() {
	FaultDrawer_SetForeColor(0xFFFF);
	FaultDrawer_SetBackColor(1);
	Fault_Sleep(0x10);
	Fault_UpdatePadImpl();
	osWritebackDCacheAll();
}