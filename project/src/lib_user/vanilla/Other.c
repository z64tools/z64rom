#include <uLib.h>

void uLib_DebugMessages(u32 msgID) {
	switch (msgID) {
		case 1: {
			ActorOverlay* overlayEntry;
			u32 i;
			
			osLibPrintf("OverlayTable %d", EXT_ACTOR_MAX);
			osLibPrintf("Start   End     SegStart SegEnd   RamAddr profile  segname\n");
			
			for (i = 0, overlayEntry = &gActorOverlayTable[0]; i < EXT_ACTOR_MAX; i++, overlayEntry++) {
				osSyncPrintf(
					"%08x %08x %08x %08x %08x %08x %s\n",
					overlayEntry->vromStart,
					overlayEntry->vromEnd,
					overlayEntry->vramStart,
					overlayEntry->vramEnd,
					overlayEntry->loadedRamAddr,
					&overlayEntry->initInfo->id,
					overlayEntry->name != NULL ? overlayEntry->name : "?"
				);
			}
			break;
		}
		
		case 2: {
			ActorOverlay* overlayEntry;
			u32 overlaySize;
			s32 i;
			
			FaultDrawer_SetCharPad(-2, 0);
			
			FaultDrawer_Printf("actor_dlftbls %u\n", gMaxActorId);
			FaultDrawer_Printf("No. RamStart- RamEnd cn  Name\n");
			
			for (i = 0, overlayEntry = &gActorOverlayTable[0]; i < gMaxActorId; i++, overlayEntry++) {
				overlaySize = (u32)overlayEntry->vramEnd - (u32)overlayEntry->vramStart;
				if (overlayEntry->loadedRamAddr != NULL) {
					FaultDrawer_Printf(
						"%3d %08x-%08x %3d %s\n",
						i,
						overlayEntry->loadedRamAddr,
						(u32)overlayEntry->loadedRamAddr + overlaySize,
						overlayEntry->numLoaded,
						overlayEntry->name != NULL ? overlayEntry->name : ""
					);
				}
			}
		}
	}
}
