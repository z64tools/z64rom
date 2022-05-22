#include <ULib.h>

/*
   z64ram = 0x80002130
   z64rom = 0x002D30
 */

void osSyncPrintf(const char* fmt, ...) {
#ifdef DEV_BUILD
	if (gLibCtx.state.vanillaOsPrintf == 0)
		return;
	va_list args;
	
	va_start(args, fmt);
	
	_Printf(is_proutSyncPrintf, NULL, fmt, args);
	
	va_end(args);
#endif
}