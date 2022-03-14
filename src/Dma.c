#include "z64rom.h"

Slot* gSlotHead;

struct {
	struct {
		s32 writable;
	} entry[4000];
	u32 highest;
} gDma;

u32 gCompressFlag = false;
u8* gYazBuf;

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
			printf_debugExt_align("DmaEntry", "Could not find");
			printf_debug("%08X - %08X", ReadBE(vromA), ReadBE(vromB));
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

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

/**
 * id < -1, write without assigning DMA entry
 * id == -1, write to first free entry
 * id > 0 write to specific entry
 */
u32 Dma_WriteEntry(Rom* rom, s32 id, MemFile* memFile, bool compress) {
	static u32 firstCompressedID = 0;
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot = gSlotHead;
	u32 fileSize = memFile->dataSize;
	u32 start;
	
	if (compress && gCompressFlag) {
		char* yazFile = Tmp_Alloc(strlen(memFile->info.name) + 0x20);
		
		if (gYazBuf == NULL) gYazBuf = Calloc(0, MbToBin(32));
		
		strcpy(yazFile, memFile->info.name);
		String_SwapExtension(yazFile, memFile->info.name, ".yaz");
		
		if (Stat(yazFile) >= memFile->info.age) {
			MemFile_LoadFile(memFile, yazFile);
		} else {
			memcpy(gYazBuf, memFile->data, memFile->dataSize);
			memFile->dataSize = Yaz_Encode(memFile->data, gYazBuf, memFile->dataSize);
			MemFile_SaveFile(memFile, yazFile);
		}
	}
	
	while (slot != NULL) {
		if (Slot_Size(slot) > memFile->dataSize + 0x10)
			break;
		slot = slot->next;
	}
	
	if (slot == NULL)
		printf_error("Could not find slot for DMA");
	
	if (id < -1) {
		start = rom->file.seekPoint = slot->romStart;
		MemFile_Append(&rom->file, memFile);
		MemFile_Align(&rom->file, 16);
		
		slot->romStart = rom->file.seekPoint;
		
		if (Slot_Size(slot) <= 0x10) {
			Node_Remove(gSlotHead, slot);
		}
		
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
			if (i > gDma.highest)
				printf_error("Coult not find free dma entry");
		}
	}
	
	if (gCompressFlag && compress && firstCompressedID == 0) {
		firstCompressedID = id;
		printf_info("Compressed DMA ID %d", firstCompressedID);
	}
	
	start = slot->romStart;
	rom->file.seekPoint = start;
	dma->vromStart = start;
	dma->romStart = start;
	
	MemFile_Append(&rom->file, memFile);
	MemFile_Align(&rom->file, 16);
	dma->vromEnd = start + fileSize;
	dma->romEnd = (start + memFile->dataSize) * (compress && gCompressFlag);
	
	SwapBE(dma->romStart);
	SwapBE(dma->romEnd);
	SwapBE(dma->vromEnd);
	SwapBE(dma->vromStart);
	slot->romStart = rom->file.seekPoint;
	
	return start;
}

void Dma_FreeEntry(Rom* rom, u32 id, u32 dmaAlign) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot;
	
	if (gDma.entry[id].writable == true) {
		printf_warning("Trying to reflag dma id [%d]", id);
		
		return;
	}
	
	gDma.entry[id].writable = true;
	if (id > gDma.highest) gDma.highest = id;
	
	if (ReadBE(dma->vromStart) - ReadBE(dma->vromEnd) == 0)
		return;
	slot = Tmp_Alloc(sizeof(struct Slot));
	slot->romStart = ReadBE(dma->vromStart);
	slot->romEnd = ReadBE(dma->vromEnd);
	
	dma->romStart = dma->romEnd = __UINT32_MAX__;
	dma->vromStart = dma->vromEnd = 0;
	
	if (dmaAlign) {
		slot->romEnd = Align(slot->romEnd, dmaAlign);
	}
	
	Node_Add(gSlotHead, slot);
}

void Dma_FreeSegment(Rom* rom, u32 romStart, u32 romEnd) {
	Slot* slot = Tmp_Alloc(sizeof(struct Slot));
	
	slot->romStart = romStart;
	slot->romEnd = romEnd;
	
	Node_Add(gSlotHead, slot);
}

void Dma_Free(Rom* rom, DmaBank type) {
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
			for (s32 i = 1518; i <= 1530; i++) {
				// Unused
				Dma_FreeEntry(rom, i, 0x1000);
			}
			Dma_FreeSegment(rom, 0x35CE000, 0x4000000);
			break;
	}
}

u32 Dma_GetRomSize(void) {
	Slot* slot = gSlotHead;
	u32 romEnd = 0;
	
	while (slot != NULL) {
		if (slot->romStart > romEnd)
			romEnd = slot->romStart;
		slot = slot->next;
	}
	
	return Align(romEnd, MbToBin(1));
}

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

s32 Dma_Intersect(Slot* a, Slot* b) {
	if (a->romStart == b->romEnd || a->romStart == b->romEnd)
		return 1;
	
	return ((Max(a->romStart, b->romStart) < Min(a->romEnd, b->romEnd)));
}

void Dma_CombineSlots(void) {
	Slot* slot = gSlotHead;
	Slot* comp;
	
	while (slot != NULL) {
		comp = gSlotHead;
		while (comp != NULL) {
			Slot* next = comp->next;
			if (comp != slot) {
				if (Dma_Intersect(slot, comp)) {
					slot->romStart = Min(slot->romStart, comp->romStart);
					slot->romEnd = Max(slot->romEnd, comp->romEnd);
					Node_Remove(gSlotHead, comp);
					comp = gSlotHead;
					continue;
				}
			}
			
			comp = next;
		}
		
		slot = slot->next;
	}
}

void Dma_PrintfSlots(Rom* rom) {
	Slot* slot = gSlotHead;
	char* color[] = {
		PRNT_REDD,
		PRNT_GREN,
		PRNT_YELW,
		PRNT_BLUE,
		PRNT_PRPL,
		PRNT_CYAN,
	};
	
	printf("[ ");
	for (f32 i = 0; i < rom->file.dataSize; i += MbToBin(1.00)) {
		u32 val = MbToBin(10.00);
		if (((u32)i % val) < 100) {
			printf("|");
		} else {
			printf(" ");
		}
	}
	printf(" ]\n");
	
	f32 i = 0;
	s32 c = 0;
	
	printf("[ ");
	while (slot != NULL) {
		s32 begn = 0;
		
		for (; i < rom->file.dataSize; i += MbToBin(1.00)) {
			if (i >= slot->romStart && (i <= slot->romEnd || begn == 0)) {
				printf("" "%s" "@", color[c % ArrayCount(color)]);
				begn = 1;
			} else if (i > slot->romEnd) {
				c++;
				break;
			} else {
				printf("" PRNT_DGRY "O");
			}
		}
		
		slot = slot->next;
	}
	printf("" PRNT_RSET " ]\n");
	
	slot = gSlotHead;
	while (slot != NULL) {
		printf("%08X-%08X " PRNT_YELW "%12.2f kB" PRNT_RSET "\n", slot->romStart, slot->romEnd, BinToKb(slot->romEnd - slot->romStart));
		
		slot = slot->next;
	}
}

void Dma_WriteFlag(u32 id, bool value) {
	gDma.entry[id].writable = value;
}

void Dma_ClearSlots() {
	gSlotHead = NULL;
}