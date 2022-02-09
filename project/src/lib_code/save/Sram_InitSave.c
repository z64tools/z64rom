#include "Sram.h"

void Sram_InitSave(FileChooseContext* fileChooseCtx, SramContext* sramCtx) {
	u16 offset;
	u16 j;
	u16* ptr;
	u16 checksum;
	
	Sram_InitNewSave();
	
	gSaveContext.entranceIndex = 0xBB;
	gSaveContext.linkAge = LINK_AGE_CHILD;
	gSaveContext.dayTime = 0x6AAB;
	gSaveContext.cutsceneIndex = 0xFFF1;
	
	if (fileChooseCtx->buttonIndex == 0) {
		gSaveContext.cutsceneIndex = 0;
	}
	
	for (offset = 0; offset < 8; offset++) {
		gSaveContext.playerName[offset] = fileChooseCtx->fileNames[fileChooseCtx->buttonIndex][offset];
	}
	
	gSaveContext.newf[0] = 'Z';
	gSaveContext.newf[1] = 'E';
	gSaveContext.newf[2] = 'L';
	gSaveContext.newf[3] = 'D';
	gSaveContext.newf[4] = 'A';
	gSaveContext.newf[5] = 'Z';
	gSaveContext.n64ddFlag = fileChooseCtx->n64ddFlag;
	
	ptr = (u16*)&gSaveContext;
	j = 0;
	checksum = 0;
	
	for (offset = 0; offset < CHECKSUM_SIZE; offset++) {
		osSyncPrintf("%x ", *ptr);
		checksum += *ptr++;
		if (++j == 0x20) {
			osSyncPrintf("\n");
			j = 0;
		}
	}
	
	gSaveContext.checksum = checksum;
	offset = gSramSlotOffsets[gSaveContext.fileNum];
	MemCopy(sramCtx->readBuff + offset, &gSaveContext, sizeof(Save));
	offset = gSramSlotOffsets[gSaveContext.fileNum + 3];
	MemCopy(sramCtx->readBuff + offset, &gSaveContext, sizeof(Save));
	
	SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), sramCtx->readBuff, SRAM_SIZE, OS_WRITE);
	
	j = gSramSlotOffsets[gSaveContext.fileNum];
	
	MemCopy(
		&fileChooseCtx->deaths[gSaveContext.fileNum],
		sramCtx->readBuff + j + DEATHS,
		sizeof(fileChooseCtx->deaths[0])
	);
	MemCopy(
		&fileChooseCtx->fileNames[gSaveContext.fileNum],
		sramCtx->readBuff + j + NAME,
		sizeof(fileChooseCtx->fileNames[0])
	);
	MemCopy(
		&fileChooseCtx->healthCapacities[gSaveContext.fileNum],
		sramCtx->readBuff + j + HEALTH_CAP,
		sizeof(fileChooseCtx->healthCapacities[0])
	);
	MemCopy(
		&fileChooseCtx->questItems[gSaveContext.fileNum],
		sramCtx->readBuff + j + QUEST,
		sizeof(fileChooseCtx->questItems[0])
	);
	MemCopy(
		&fileChooseCtx->n64ddFlags[gSaveContext.fileNum],
		sramCtx->readBuff + j + N64DD,
		sizeof(fileChooseCtx->n64ddFlags[0])
	);
	MemCopy(
		&fileChooseCtx->defense[gSaveContext.fileNum],
		sramCtx->readBuff + j + DEFENSE,
		sizeof(fileChooseCtx->defense[0])
	);
	MemCopy(
		&fileChooseCtx->health[gSaveContext.fileNum],
		sramCtx->readBuff + j + HEALTH,
		sizeof(fileChooseCtx->health[0])
	);
}