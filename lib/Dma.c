#include "z64rom.h"

Slot* gSlotHead;

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
		romFile.size = CLAMP_MIN((s32)romFile.romEnd - (s32)romFile.romStart, 0);
		
		return romFile;
	}
	
	romFile.romStart = ReadBE(dmaTable[i].romStart);
	if (dmaTable[i].romEnd != 0)
		romFile.romEnd = ReadBE(dmaTable[i].romEnd);
	else
		romFile.romEnd = ReadBE(dmaTable[i].vromEnd);
	romFile.data = SegmentedToVirtual(0x0, romFile.romStart);
	romFile.size = CLAMP_MIN((s32)romFile.romEnd - (s32)romFile.romStart, 0);
	
	return romFile;
}

#define Dma_RomFile_Func(type, PART, name) \
	Dma_RomFile_ ## name(Rom * rom, s32 id) { \
		type* entry = &rom->PART[id]; \
		return Rom_GetRomFile(rom, entry->vromStart, entry->vromEnd); \
	}

RomFile Dma_RomFile_Func(ObjectEntry, table.object, Object);
RomFile Dma_RomFile_Func(ActorEntry, table.actor, Actor);
RomFile Dma_RomFile_Func(DmaEntry, table.dma, DmaEntry);
RomFile Dma_RomFile_Func(GameStateEntry, table.state, GameState);
RomFile Dma_RomFile_Func(SceneEntry, table.scene, Scene);

static u32 Slot_Size(Slot* slot) {
	return slot->romEnd - slot->romStart;
}

void Dma_CombineSlots(void) {
	Slot* slot = gSlotHead;
	Slot* comp;
	
	while (slot != NULL) {
		comp = gSlotHead;
		while (comp != NULL) {
			if (slot->romEnd == comp->romStart) {
				slot->romEnd = comp->romEnd;
				Node_Remove(gSlotHead, comp);
				break;
			}
			
			if (slot->romStart == comp->romEnd) {
				slot->romStart = comp->romStart;
				Node_Remove(gSlotHead, comp);
				break;
			}
			comp = comp->next;
		}
		slot = slot->next;
	}
}

void Dma_PrintfSlots(Rom* rom) {
	Slot* slot = gSlotHead;
	
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
	
	while (slot != NULL) {
		printf("[ ");
		s32 begn = 0;
		
		for (f32 i = 0; i < rom->file.dataSize; i += MbToBin(1.00)) {
			if (i >= slot->romStart && (i <= slot->romEnd || begn == 0)) {
				printf("" PRNT_BLUE "@");
				begn = 1;
			} else {
				printf("" PRNT_DGRY "O");
			}
		}
		printf("" PRNT_RSET " ]" "[%08X - %08X]\n", slot->romStart, slot->romEnd);
		
		slot = slot->next;
	}
}

u32 Dma_WriteEntry(Rom* rom, s32 id, MemFile* memFile) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot = gSlotHead;
	u32 size = memFile->dataSize;
	
	while (slot != NULL) {
		if (Slot_Size(slot) > size + 0x10)
			break;
		slot = slot->next;
	}
	
	if (slot == NULL)
		printf_error("Could not find slot for DMA");
	
	if (id < 0) {
		u32 start = rom->file.seekPoint = slot->romStart;
		MemFile_Append(&rom->file, memFile);
		MemFile_Align(&rom->file, 16);
		
		slot->romStart = rom->file.seekPoint;
		
		if (Slot_Size(slot) <= 0x10) {
			Node_Remove(gSlotHead, slot);
		}
		
		return start;
	}
	
	dma->vromStart = slot->romStart;
	dma->romStart = slot->romStart;
	rom->file.seekPoint = slot->romStart;
	MemFile_Append(&rom->file, memFile);
	MemFile_Align(&rom->file, 16);
	
	dma->vromEnd = rom->file.seekPoint;
	dma->romEnd = 0;
	SwapBE(dma->romStart);
	SwapBE(dma->vromStart);
	SwapBE(dma->vromEnd);
	
	slot->romStart = rom->file.seekPoint;
	
	if (Slot_Size(slot) <= 0x10) {
		Node_Remove(gSlotHead, slot);
	}
	
	return ReadBE(dma->vromStart);
}

void Dma_FreeEntry(Rom* rom, u32 id, u32 dmaAlign) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot slot = { 0 };
	Slot* node;
	Slot* compSlot = gSlotHead;
	
	slot.romStart = ReadBE(dma->vromStart);
	slot.romEnd = ReadBE(dma->vromEnd);
	
	dma->romStart = dma->romEnd = 0xFFFFFFFF;
	dma->vromStart = dma->vromEnd = 0;
	
	if (dmaAlign) {
		if ((slot.romEnd % dmaAlign) != 0) {
			slot.romEnd = slot.romEnd - (slot.romEnd % dmaAlign) + dmaAlign;
		}
	}
	
	if (Slot_Size(&slot) == 0)
		return;
	
	// Combine freed slots
	while (compSlot != NULL) {
		if (compSlot->romStart == slot.romEnd) {
			compSlot->romStart = slot.romStart;
			
			return;
		}
		if (compSlot->romEnd == slot.romStart) {
			compSlot->romEnd = slot.romEnd;
			
			return;
		}
		
		compSlot = compSlot->next;
	}
	
	node = Graph_Alloc(sizeof(struct Slot));
	
	memcpy(node, &slot, sizeof(struct Slot));
	Node_Add(gSlotHead, node);
}

void Dma_FreeSegment(Rom* rom, u32 romStart, u32 romEnd) {
	Slot* slot = Graph_Alloc(sizeof(struct Slot));
	Slot* compSlot = gSlotHead;
	
	slot->romStart = romStart;
	slot->romEnd = romEnd;
	
	// Combine freed slots
	while (compSlot != NULL) {
		if (compSlot->romEnd == slot->romStart) {
			compSlot->romEnd = slot->romEnd;
			
			return;
		}
		
		compSlot = compSlot->next;
	}
	
	Node_Add(gSlotHead, slot);
}

void Dma_Free(Rom* rom, DmaBank type) {
	switch (type) {
		case DMA_AUDIO:
			for (s32 i = 3; i <= 5; i++) {
				// Audio Entries
				Dma_FreeEntry(rom, i, 0x10);
			}
			break;
		case DMA_ACTOR:
			for (s32 i = 36; i <= 497; i++) {
				// Actor Entries
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
		case DMA_BOXTEX:
			for (s32 i = 941; i <= 1004; i++) {
				// Box textures (sky / bg) Entries
				Dma_FreeEntry(rom, i, 0x1000);
			}
			break;
		case DMA_SCENES:
			for (s32 i = 1007; i <= 1517; i++) {
				// Scene Entries
				Dma_FreeEntry(rom, i, 0x1000);
			}
			break;
		case DMA_UNUSED:
			for (s32 i = 1519; i <= 1530; i++) {
				// Unused
				Dma_FreeEntry(rom, i, 0x1000);
			}
			Dma_FreeSegment(rom, 0x35CE040, 0x4000000);
			break;
	}
	Dma_CombineSlots();
}