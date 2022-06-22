#include "z64rom.h"

Slot* gSlotHead;
Slot* gSlotYazHead;

struct {
	struct {
		u8 writable : 1;
		u8 compress : 1;
	} entry[8000];
	u32 highest;
} gDma;

u32 sVromEnd;
u8* gYazBuf;
s32 gEntries;

RomFile Rom_GetRomFile(Rom* rom, u32 vromA, u32 vromB) {
	DmaEntry* dmaTable = rom->table.dma;
	s32 i;
	RomFile romFile;
	u32 useAddress = false;
	
	for (i = 0; i < rom->table.num.dma; i++) {
		if (dmaTable[i].vromStart == vromA &&
			dmaTable[i].vromEnd == vromB) {
			break;
		}
		if (i + 1 == rom->table.num.dma) {
			useAddress = true;
		}
	}
	
	if (useAddress) {
		romFile.romStart = ReadBE(vromA);
		romFile.romEnd = ReadBE(vromB);
		romFile.data = SegmentedToVirtual(0x0, romFile.romStart);
		romFile.size = ClampMin((s32)romFile.romEnd - (s32)romFile.romStart, 0);
		
		return romFile;
	}
	
	romFile.romStart = ReadBE(dmaTable[i].romStart);
	if (dmaTable[i].romEnd != 0)
		romFile.romEnd = ReadBE(dmaTable[i].romEnd);
	else
		romFile.romEnd = ReadBE(dmaTable[i].vromEnd);
	romFile.data = SegmentedToVirtual(0x0, romFile.romStart);
	romFile.size = ClampMin((s32)romFile.romEnd - (s32)romFile.romStart, 0);
	
	return romFile;
}

#define Dma_RomFile_Func(type, PART, name) \
	Dma_RomFile_ ## name(Rom * rom, s32 id) { \
		type* entry = &rom->PART[id]; \
		return Rom_GetRomFile(rom, entry->vromStart, entry->vromEnd); \
	}

RomFile Dma_RomFile_Func(ObjectEntry, table.object, Object)
RomFile Dma_RomFile_Func(ActorEntry, table.actor, Actor)
RomFile Dma_RomFile_Func(EffectEntry, table.effect, Effect)
RomFile Dma_RomFile_Func(DmaEntry, table.dma, DmaEntry)
RomFile Dma_RomFile_Func(GameStateEntry, table.state, GameState)
RomFile Dma_RomFile_Func(SceneEntry, table.scene, Scene)

static u32 Slot_Size(Slot* slot) {
	return slot->romEnd - slot->romStart;
}

static s32 Dma_Intersect(Slot* a, Slot* b) {
	if (a->romStart == b->romEnd || a->romStart == b->romEnd)
		return 1;
	
	return ((Max(a->romStart, b->romStart) < Min(a->romEnd, b->romEnd)));
}

static Slot* Slot_GetFree(Slot* slot, Size size) {
	while (slot != NULL) {
		if (Slot_Size(slot) > size + 0x10)
			break;
		slot = slot->next;
	}
	
	if (slot == NULL)
		printf_error("Could not find slot for DMA");
	
	return slot;
}

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

/**
 * id < -1, write without assigning DMA entry
 * id == -1, write to first free entry
 * id > 0 write to specific entry
 */
u32 Dma_WriteEntry(Rom* rom, s32 id, MemFile* memFile, s32 compress) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot = NULL;
	Slot* yazt = NULL;
	u32 size = memFile->dataSize;
	u32 start;
	
	if (compress && gCompressFlag) {
		if (gYazBuf == NULL)
			Calloc(gYazBuf, MbToBin(32));
		
		if (compress == NOCACHE_COMPRESS) {
			memcpy(gYazBuf, memFile->data, memFile->dataSize);
			memFile->dataSize = Yaz_Encode(memFile->data, gYazBuf, memFile->dataSize);
		} else {
			char* yazFile = xAlloc(strlen(memFile->info.name) + 0x80);
			
			strcpy(yazFile, memFile->info.name);
			StrRep(yazFile, "rom/", "rom/yaz-cache/");
			strcat(yazFile, ".yaz");
			
			if (Sys_Stat(yazFile) >= memFile->info.age) {
				MemFile_LoadFile(memFile, yazFile);
			} else {
				Sys_MakeDir(Path(yazFile));
				memcpy(gYazBuf, memFile->data, memFile->dataSize);
				memFile->dataSize = Yaz_Encode(memFile->data, gYazBuf, memFile->dataSize);
				MemFile_SaveFile(memFile, yazFile);
			}
		}
	}
	
	slot = Slot_GetFree(gSlotHead, size);
	if (gCompressFlag)
		yazt = Slot_GetFree(gSlotYazHead, memFile->dataSize);
	
	if (id < -1) {
		start = rom->file.seekPoint = slot->romStart;
		MemFile_Append(&rom->file, memFile);
		MemFile_Align(&rom->file, 16);
		
		slot->romStart += Align(size, 16);
		if (gCompressFlag)
			yazt->romStart += Align(memFile->dataSize, 16);
		
		if (Slot_Size(slot) <= 0x10)
			Node_Remove(gSlotHead, slot);
		
		if (gCompressFlag)
			if (Slot_Size(yazt) <= 0x10)
				Node_Remove(gSlotYazHead, yazt);
		
		return start;
	}
	
	if (id == -1) {
		for (s32 i = 0;; i++) {
			if (gDma.entry[i].writable) {
				gDma.entry[i].writable = false;
				dma = &rom->table.dma[i];
				id = i;
				break;
			}
			if (i > gDma.highest) {
				printf_error("Ran out of free DMA entries! Run " PRNT_BLUE "[z64rom.exe --no-beta]" PRNT_RSET " to remove unused content");
			}
		}
	} else {
		gDma.entry[id].writable = false;
	}
	
	if (gCompressFlag) {
		rom->file.seekPoint = yazt->romStart;
		MemFile_Append(&rom->file, memFile);
		MemFile_Align(&rom->file, 16);
		
		start = slot->romStart;
		dma->vromStart = slot->romStart;
		dma->vromEnd = dma->vromStart + size;
		
		dma->romStart = yazt->romStart;
		dma->romEnd = (yazt->romStart + Align(memFile->dataSize, 16)) * compress;
		sVromEnd = dma->vromEnd;
		
		SwapBE(dma->romStart);
		SwapBE(dma->romEnd);
		SwapBE(dma->vromEnd);
		SwapBE(dma->vromStart);
		
		slot->romStart += Align(size, 16);
		yazt->romStart += Align(memFile->dataSize, 16);
		
		gEntries--;
		
		return start;
	} else {
		start = slot->romStart;
		rom->file.seekPoint = start;
		
		MemFile_Append(&rom->file, memFile);
		MemFile_Align(&rom->file, 16);
		
		dma->vromStart = start;
		dma->vromEnd = start + memFile->dataSize;
		dma->romStart = start;
		dma->romEnd = 0;
		
		SwapBE(dma->romStart);
		SwapBE(dma->romEnd);
		SwapBE(dma->vromEnd);
		SwapBE(dma->vromStart);
		slot->romStart = rom->file.seekPoint;
		
		sVromEnd = start + memFile->dataSize;
		
		gEntries--;
		
		return start;
	}
}

u32 Dma_AllocEntry(Rom* rom, s32 id, Size size) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot = NULL;
	Slot* yazt = NULL;
	u32 start;
	
	slot = Slot_GetFree(gSlotHead, size);
	if (gCompressFlag)
		yazt = Slot_GetFree(gSlotYazHead, size);
	
	if (id < -1) {
		start = slot->romStart;
		
		slot->romStart = start + size;
		if (gCompressFlag)
			yazt->romStart = start + size;
		
		if (Slot_Size(slot) <= 0x10)
			Node_Remove(gSlotHead, slot);
		
		if (gCompressFlag)
			if (Slot_Size(yazt) <= 0x10)
				Node_Remove(gSlotYazHead, yazt);
		
		return start;
	}
	
	if (id == -1) {
		for (s32 i = 0;; i++) {
			if (gDma.entry[i].writable) {
				gDma.entry[i].writable = false;
				dma = &rom->table.dma[i];
				id = i;
				break;
			}
			if (i > gDma.highest) {
				printf_error("Ran out of free DMA entries! Run " PRNT_BLUE "[z64rom.exe --no-beta]" PRNT_RSET " to remove unused content");
			}
		}
	}
	
	gDma.entry[id].writable = false;
	
	start = slot->romStart;
	
	dma->vromStart = start;
	dma->vromEnd = start + size;
	dma->romStart = gCompressFlag ? yazt->romStart : slot->romStart;
	dma->romEnd = 0;
	
	SwapBE(dma->romStart);
	SwapBE(dma->romEnd);
	SwapBE(dma->vromEnd);
	SwapBE(dma->vromStart);
	
	slot->romStart = Align(start + size, 16);
	if (gCompressFlag)
		yazt->romStart = Align(start + size, 16);
	
	sVromEnd = start + size;
	
	gEntries--;
	
	return start;
}

void Dma_UpdateRomSize(Rom* rom) {
	Slot* slot = gCompressFlag ? gSlotYazHead : gSlotHead;
	
	while (slot->next)
		slot = slot->next;
	
	rom->file.dataSize = slot->romStart;
	rom->file.dataSize = Align(rom->file.dataSize, 0x1000);
}

u32 Dma_GetVRomEnd(void) {
	return sVromEnd;
}

// # # # # # # # # # # # # # # # # # # # #
// # FREE                                #
// # # # # # # # # # # # # # # # # # # # #

void Dma_FreeEntry(Rom* rom, u32 id, u32 dmaAlign) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot;
	Slot* yazt;
	
	if (gDma.entry[id].writable == true) {
		printf_warning("Trying to reflag dma id [%d]", id);
		
		return;
	}
	
	gDma.entry[id].writable = true;
	gDma.highest = Max(gDma.highest, id);
	gEntries++;
	
	if (ReadBE(dma->vromStart) - ReadBE(dma->vromEnd) == 0) {
		*dma = (DmaEntry) { -1, -1, -1, -1 };
		
		return;
	}
	
	slot = xAlloc(sizeof(struct Slot));
	slot->romStart = ReadBE(dma->vromStart);
	slot->romEnd = ReadBE(dma->vromEnd);
	
	*dma = (DmaEntry) { -1, -1, -1, -1 };
	
	if (dmaAlign)
		slot->romEnd = Align(slot->romEnd, dmaAlign);
	
	Node_Add(gSlotHead, slot);
	
	if (!gCompressFlag)
		return;
	
	yazt = xAlloc(sizeof(struct Slot));
	yazt->romStart = slot->romStart;
	yazt->romEnd = slot->romEnd;
	Node_Add(gSlotYazHead, yazt);
}

void Dma_FreeSegment(Rom* rom, u32 romStart, u32 romEnd) {
	Slot* slot = xAlloc(sizeof(struct Slot));
	
	slot->romStart = romStart;
	slot->romEnd = romEnd;
	
	Node_Add(gSlotHead, slot);
	
	if (!gCompressFlag)
		return;
	
	slot = xAlloc(sizeof(struct Slot));
	slot->romStart = romStart;
	slot->romEnd = romEnd;
	Node_Add(gSlotYazHead, slot);
}

void Dma_FreeGroup(Rom* rom, DmaBank type) {
	switch (type) {
		case DMA_ACTOR:
			for (s32 i = 36; i <= 198; i++) {
				// Actor Entries
				Dma_FreeEntry(rom, i, 0x10);
			}
			for (s32 i = 235; i <= 497; i++) {
				// Actor Entries
				Dma_FreeEntry(rom, i, 0x10);
			}
			break;
		case DMA_STATE:
			for (s32 i = 29; i <= 32; i++) {
				// Actor Entries
				Dma_FreeEntry(rom, i, 0x10);
			}
			break;
		case DMA_KALEIDO:
			for (s32 i = 33; i <= 34; i++) {
				// Actor Entries
				Dma_FreeEntry(rom, i, 0x10);
			}
			break;
		case DMA_EFFECT:
			for (s32 i = 199; i <= 234; i++) {
				// Effect Entries
				Dma_FreeEntry(rom, i, 0x10);
			}
			break;
		case DMA_OBJECT:
			for (s32 i = 498; i <= 879; i++) {
				// Object Entries
				Dma_FreeEntry(rom, i, 0x1000);
			}
			break;
		case DMA_PLACE_NAME:
			for (s32 i = 880; i <= 936; i++) {
				// Place Name Entries
				Dma_FreeEntry(rom, i, 0x1000);
			}
			break;
		case DMA_SKYBOX_TEXEL:
			for (s32 i = 941; i <= 1004; i++) {
				// Box textures (sky / bg) Entries
				Dma_FreeEntry(rom, i, 0x1000);
				Dma_WriteFlag(i, false);
			}
			break;
		case DMA_SCENES:
			for (s32 i = 1007; i <= 1517; i++) {
				// Scene Entries
				Dma_FreeEntry(rom, i, 0x1000);
			}
			break;
		case DMA_UNUSED:
			for (s32 i = 1518; i < rom->table.num.dma; i++) {
				// Unused
				Dma_FreeEntry(rom, i, 0x1000);
			}
			for (s32 i = rom->table.num.dma; i < rom->ext.dmaNum; i++) {
				gDma.entry[i].writable = true;
				gEntries++;
				gDma.highest = i;
			}
			
			Dma_FreeSegment(rom, 0x35CF000, 0x4000000);
			break;
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # OTHER                               #
// # # # # # # # # # # # # # # # # # # # #

void Dma_CombineSlots(void) {
	Slot* slot;
	u32 run = true;
	
	// Sort
	while (run) {
		slot = gSlotHead;
		run = false;
		
		while (slot) {
			Slot* next = slot->next;
			// Swap With Next
			if (next && next->romStart < slot->romStart) {
				Swap(slot->romStart, next->romStart);
				Swap(slot->romEnd, next->romEnd);
				
				run = true;
			}
			
			slot = next;
		}
	}
	
	slot = gSlotHead;
	while (slot) {
		Slot* next = slot->next;
		
		if (next && slot->romEnd == next->romStart) {
			slot->romEnd = next->romEnd;
			Node_Remove(gSlotHead, next);
			continue;
		}
		
		slot = slot->next;
	}
	
	if (gCompressFlag) {
		Slot* slot;
		u32 run = true;
		
		// Sort
		while (run) {
			slot = gSlotYazHead;
			run = false;
			
			while (slot) {
				Slot* next = slot->next;
				// Swap With Next
				if (next && next->romStart < slot->romStart) {
					Swap(slot->romStart, next->romStart);
					Swap(slot->romEnd, next->romEnd);
					
					run = true;
				}
				
				slot = next;
			}
		}
		
		slot = gSlotYazHead;
		while (slot) {
			Slot* next = slot->next;
			
			if (next && slot->romEnd == next->romStart) {
				slot->romEnd = next->romEnd;
				Node_Remove(gSlotYazHead, next);
				
				slot = gSlotYazHead;
				continue;
			}
			
			slot = slot->next;
		}
	}
}

void Dma_PrintfSlots(Rom* rom, const char* message, Slot* head) {
	Slot* slot = head;
	char* color[] = {
		PRNT_REDD,
		PRNT_GREN,
		PRNT_YELW,
		PRNT_BLUE,
		PRNT_PRPL,
		PRNT_CYAN,
	};
	
	printf("\n");
	printf_warning("Slots: %s", message);
	printf("" PRNT_DGRY "[" PRNT_YELW ">" PRNT_DGRY "]: [ " PRNT_GRAY);
	for (f32 i = 0; i < rom->file.dataSize; i += MbToBin(1.00)) {
		u32 val = MbToBin(10.00);
		if (((u32)i % val) < 100) {
			printf("|");
		} else {
			printf(" ");
		}
	}
	printf("" PRNT_DGRY " ]\n");
	
	f32 i = 0;
	s32 c = 0;
	
	printf("" PRNT_DGRY "[" PRNT_YELW ">" PRNT_DGRY "]: [ ");
	while (slot != NULL) {
		s32 begn = 0;
		
		for (; i < rom->file.dataSize; i += MbToBin(1.00)) {
			if (i >= slot->romStart && (i <= slot->romEnd || begn == 0)) {
				printf("" "%s" "=", color[c % ArrayCount(color)]);
				begn = 1;
			} else if (i > slot->romEnd) {
				c++;
				break;
			} else {
				printf("" PRNT_DGRY "-");
			}
		}
		
		slot = slot->next;
	}
	printf("" PRNT_DGRY " ]\n");
	printf_warning("DMA Entries: %d\n", gEntries);
	
	slot = head;
	while (slot != NULL) {
		if (BinToMb(slot->romEnd - slot->romStart) < 1.0f)
			printf("%08X-%08X " PRNT_GREN "%12.2f kB" PRNT_RSET "\n", slot->romStart, slot->romEnd, BinToKb(slot->romEnd - slot->romStart));
		else
			printf("%08X-%08X " PRNT_CYAN "%12.2f mB" PRNT_RSET "\n", slot->romStart, slot->romEnd, BinToMb(slot->romEnd - slot->romStart));
		
		slot = slot->next;
	}
}

void Dma_WriteFlag(u32 id, bool value) {
	gDma.entry[id].writable = value;
}