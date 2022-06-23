#include <uLib.h>

void* Segment_Scene_GetHeader(void* segment, s32 setupIndex) {
	u32* setupAddr = segment;
	
	if (*setupAddr == 0x18000000 && setupIndex != 0) {
		u32* headers_list = (u32*)*(setupAddr + 1);
		
		for (s32 i = setupIndex - 1; i > -1; i--) {
			u32* addr = (u32*)SEGMENTED_TO_VIRTUAL((u32)(headers_list + i));
			
			if (*addr != 0) {
				setupAddr = (u32*)SEGMENTED_TO_VIRTUAL((u32) * addr);
				break;
			}
		}
	}
	
	return setupAddr;
}

void* Segment_Scene_GetCutscene(void* segment, s32 setupIndex) {
	u32* setupAddr = Segment_Scene_GetHeader(segment, setupIndex);
	
	for (s32 i = 0; i < 25; i++) {
		if (*((u8*)(setupAddr + i * 2)) == 0x14)
			break;
		
		if (*(setupAddr + (i * 2)) == 0x17000000)
			return (u32*)*(setupAddr + (i * 2) + 1);
	}
	
	return 0;
}

void* Segment_Scene_GetPathList(void* segment, s32 setupIndex) {
	u32* setupAddr = Segment_Scene_GetHeader(segment, setupIndex);
	
	while (*setupAddr != 0x14000000) {
		if (*setupAddr == 0x0D000000)
			return (u32*)SEGMENTED_TO_VIRTUAL(setupAddr[1]);
		
		setupAddr += 2;
	}
	
	return 0;
}

void* Segment_Scene_GetPath(u32* pathListAddr, s16 pathID) {
	return (u32*)(pathListAddr + (pathID * 2));
}

u8 Segment_Scene_GetPathNodeNum(u32* pathAddr) {
	return (u8) * ((u8*)pathAddr);
}

Vec3f* Segment_Scene_GetPathVec3f(u32* pathListAddr, s16 pathID, s16 nodeID) {
	u32* pathAddr = Segment_Scene_GetPath(pathListAddr, pathID);
	u8 numNodes = Segment_Scene_GetPathNodeNum(pathAddr);
	
	if (numNodes < nodeID)
		return 0;
	else
		return (Vec3f*)((SEGMENTED_TO_VIRTUAL((u32) * (pathAddr + 1))) + (nodeID * 6));
}

CollisionHeader* Segment_Scene_GetCollision(void* segment, s32 setupIndex) {
	u32* headerAddr = Segment_Scene_GetHeader(segment, setupIndex);
	
	for (s32 i = 0; i < 25; i++) {
		if (*((u8*)(headerAddr + i * 2)) == 0x14)
			break;
		
		if (*(headerAddr + (i * 2)) == 0x03000000)
			return (CollisionHeader*)SEGMENTED_TO_VIRTUAL(*(headerAddr + (i * 2) + 1));
	}
	
	return NULL;
}

void Segment_Scene_PlayCutscene(void* segment, s32 setupIndex) {
	Cutscene_SetSegment(Effect_GetPlayState(), Segment_Scene_GetCutscene(segment, setupIndex));
	gSaveContext.cutsceneTrigger = true;
}