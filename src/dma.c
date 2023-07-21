#include "z64rom.h"

Slot* gSlotHead;
Slot* gSlotYazHead;

DmaContext gDma;

u32 sVromEnd;
s32 gEntries;
s32 gNoCompressCache;

RomFile Rom_GetRomFile(Rom* rom, u32 vromA, u32 vromB) {
	DmaEntry* dmaTable = rom->table.dma;
	s32 i;
	RomFile romFile;
	u32 useAddress = false;
	
	for (i = 0; i < rom->table.num.dma; i++) {
		if (dmaTable[i].vromStart == vromA &&
			dmaTable[i].vromEnd == vromB)
			break;
		
		if (i + 1 == rom->table.num.dma)
			useAddress = true;
	}
	
	if (useAddress) {
		romFile.romStart = ReadBE(vromA);
		romFile.romEnd = ReadBE(vromB);
		romFile.data = SegmentToVirtual(0x0, romFile.romStart);
		romFile.size = clamp_min((s32)romFile.romEnd - (s32)romFile.romStart, 0);
		
		return romFile;
	}
	
	romFile.romStart = ReadBE(dmaTable[i].romStart);
	if (dmaTable[i].romEnd != 0)
		romFile.romEnd = ReadBE(dmaTable[i].romEnd);
	else
		romFile.romEnd = ReadBE(dmaTable[i].vromEnd);
	romFile.data = SegmentToVirtual(0x0, romFile.romStart);
	romFile.size = clamp_min((s32)romFile.romEnd - (s32)romFile.romStart, 0);
	
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

static Rom* s_rom;

static Slot* Slot_GetFree(Slot* slot, size_t size) {
	while (slot != NULL) {
		if (Slot_Size(slot) > size + 16)
			break;
		slot = slot->next;
	}
	
	if (slot == NULL) {
		Dma_SlotInfo(s_rom, "DMA", gSlotHead);
		errr("Could not find slot for DMA for item size of: %.2f kB", BinToKb(size));
	}
	
	return slot;
}

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

/**
 * id < -1, write without assigning DMA entry
 * id == -1, write to first free entry
 * id >= 0 write to specific entry
 */
u32 Dma_Write(Rom* rom, s32 id, void* data, u32 size, const char* name, s32 compress) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot = NULL;
	Slot* yazt = NULL;
	u32 og_size = size;
	u32 start;
	u8* yazBuf = NULL;
	const char* yazFile = NULL;
	Memfile bufmem = Memfile_New();
	
	s_rom = rom;
	
	if (compress && g64.compress) {
		osAssert((yazBuf = calloc(size * 2)) != NULL);
		
		if (compress == NOCACHE_COMPRESS || gNoCompressCache || !name) {
			size = Yaz_Encode(yazBuf, data, size);
			data = yazBuf;
		} else {
			yazFile = Yaz_Filename(name);
			
			if (sys_stat(yazFile) >= sys_stat(name)) {
				Memfile_LoadBin(&bufmem, yazFile);
				data = bufmem.data;
				size = bufmem.size;
			} else {
				sys_mkdir(x_path(yazFile));
				size = Yaz_Encode(yazBuf, data, size);
				data = yazBuf;
				
				Memfile_LoadMem(&bufmem, data, size);
				Memfile_SaveBin(&bufmem, yazFile);
				bufmem = Memfile_New();
			}
		}
	}
	
	slot = Slot_GetFree(gSlotHead, og_size);
	if (g64.compress)
		yazt = Slot_GetFree(gSlotYazHead, size);
	
	// no Entry
	if (id < -1) {
		start = rom->file.seekPoint = slot->romStart;
		Memfile_Write(&rom->file, data, size);
		Memfile_Align(&rom->file, 16);
		
		slot->romStart += alignvar(og_size, 16);
		if (g64.compress)
			yazt->romStart += alignvar(size, 16);
		
		if (Slot_Size(slot) <= 16)
			Node_Kill(gSlotHead, slot);
		
		if (g64.compress)
			if (Slot_Size(yazt) <= 16)
				Node_Kill(gSlotYazHead, yazt);
		
		goto output;
	}
	// free Entry
	else if (id == -1) {
		for (int i = 0;; i++) {
			if (gDma.entry[i].writable) {
				dma = &rom->table.dma[i];
				id = i;
				break;
			}
			if (i > gDma.highest) {
				errr("Ran out of free DMA entries! Run " PRNT_BLUE
					"[z64rom.exe --no-beta]" PRNT_RSET
					" to remove unused content");
			}
		}
	}
	
	gDma.entry[id].writable = false;
	
	if (g64.compress) {
		rom->file.seekPoint = yazt->romStart;
		Memfile_Write(&rom->file, data, size);
		Memfile_Align(&rom->file, 16);
		
		start = slot->romStart;
		dma->vromStart = slot->romStart;
		dma->vromEnd = alignvar(dma->vromStart + og_size, 16);
		
		dma->romStart = yazt->romStart;
		dma->romEnd = (yazt->romStart + alignvar(size, 16)) * compress;
		sVromEnd = dma->vromEnd;
		
		SwapBE(dma->romStart);
		SwapBE(dma->romEnd);
		SwapBE(dma->vromEnd);
		SwapBE(dma->vromStart);
		
		slot->romStart += alignvar(og_size, 16);
		yazt->romStart += alignvar(size, 16);
		
	} else {
		start = slot->romStart;
		rom->file.seekPoint = start;
		
		Memfile_Write(&rom->file, data, size);
		Memfile_Align(&rom->file, 16);
		
		dma->vromStart = start;
		dma->vromEnd = alignvar(start + size, 16);
		dma->romStart = start;
		dma->romEnd = 0;
		sVromEnd = dma->vromEnd;
		
		SwapBE(dma->romStart);
		SwapBE(dma->romEnd);
		SwapBE(dma->vromEnd);
		SwapBE(dma->vromStart);
		slot->romStart = rom->file.seekPoint;
	}
	
	gEntries--;
	gDma.written++;
	
	output:
	Memfile_Free(&bufmem);
	delete(yazBuf, yazFile);
	
	return start;
}

u32 Dma_WriteMemfile(Rom* rom, s32 id, Memfile* memFile, s32 compress) {
	return Dma_Write(rom, id, memFile->data, memFile->size, memFile->info.name, compress);
}

u32 Dma_AllocEntry(Rom* rom, s32 id, size_t size) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot = NULL;
	Slot* yazt = NULL;
	u32 start;
	
	slot = Slot_GetFree(gSlotHead, size);
	if (g64.compress)
		yazt = Slot_GetFree(gSlotYazHead, size);
	
	if (id < -1) {
		start = slot->romStart;
		
		slot->romStart = start + size;
		if (g64.compress)
			yazt->romStart = start + size;
		
		if (Slot_Size(slot) <= 16)
			Node_Kill(gSlotHead, slot);
		
		if (g64.compress)
			if (Slot_Size(yazt) <= 16)
				Node_Kill(gSlotYazHead, yazt);
		
		return start;
	}
	
	if (id == -1) {
		for (int i = 0;; i++) {
			if (gDma.entry[i].writable) {
				gDma.entry[i].writable = false;
				dma = &rom->table.dma[i];
				id = i;
				break;
			}
			if (i > gDma.highest) {
				errr("Ran out of free DMA entries! Run " PRNT_BLUE "[z64rom.exe --no-beta]" PRNT_RSET " to remove unused content");
			}
		}
	}
	
	gDma.entry[id].writable = false;
	
	start = slot->romStart;
	
	dma->vromStart = start;
	dma->vromEnd = start + size;
	dma->romStart = g64.compress ? yazt->romStart : slot->romStart;
	dma->romEnd = 0;
	
	SwapBE(dma->romStart);
	SwapBE(dma->romEnd);
	SwapBE(dma->vromEnd);
	SwapBE(dma->vromStart);
	
	slot->romStart = alignvar(start + size, 16);
	if (g64.compress)
		yazt->romStart = alignvar(start + size, 16);
	
	sVromEnd = start + size;
	
	gEntries--;
	
	return start;
}

void Dma_UpdateRomSize(Rom* rom) {
	Slot* slot = g64.compress ? gSlotYazHead : gSlotHead;
	
	while (slot->next)
		slot = slot->next;
	
	rom->file.size = slot->romStart;
	rom->file.size = alignvar(rom->file.size, 0x1000);
}

u32 Dma_GetVRomEnd(void) {
	return sVromEnd;
}

// # # # # # # # # # # # # # # # # # # # #
// # FREE                                #
// # # # # # # # # # # # # # # # # # # # #

static void Dma_Allocate(s32 accessID) {
	if (gDma.entry == NULL)
		gDma.entry = calloc(sizeof(*gDma.entry) * (gDma.num = 16000));
	
	if (accessID >= gDma.num) {
		s32 nnum = accessID + 0x10;
		
		gDma.entry = realloc(gDma.entry, sizeof(*gDma.entry) * nnum);
		memset(&gDma.entry[gDma.num], 0, sizeof(*gDma.entry) * (nnum - gDma.num));
		
		gDma.num = nnum;
	}
}

void Dma_RegEntry(Rom* rom, u32 id, u32 dmaAlign) {
	DmaEntry* dma = &rom->table.dma[id];
	Slot* slot;
	Slot* yazt;
	
	Dma_Allocate(id);
	
	if (gDma.entry[id].writable == true) {
		warn("Trying to reflag dma id [%d]", id);
		
		return;
	}
	
	gDma.entry[id].writable = true;
	gDma.highest = Max(gDma.highest, id);
	gEntries++;
	
	if (ReadBE(dma->vromStart) - ReadBE(dma->vromEnd) == 0) {
		*dma = (DmaEntry) { -1, -1, -1, -1 };
		
		return;
	}
	
	slot = new(Slot);
	slot->romStart = ReadBE(dma->vromStart);
	slot->romEnd = ReadBE(dma->vromEnd);
	
	*dma = (DmaEntry) { -1, -1, -1, -1 };
	
	if (dmaAlign)
		slot->romEnd = alignvar(slot->romEnd, dmaAlign);
	
	Node_Add(gSlotHead, slot);
	
	if (!g64.compress)
		return;
	
	yazt = new(Slot);
	yazt->romStart = slot->romStart;
	yazt->romEnd = slot->romEnd;
	Node_Add(gSlotYazHead, yazt);
}

void Dma_RegSegment(Rom* rom, u32 romStart, u32 romEnd) {
	Slot* slot = new(Slot);
	
	slot->romStart = romStart;
	slot->romEnd = romEnd;
	
	Node_Add(gSlotHead, slot);
	
	if (!g64.compress)
		return;
	
	slot = new(Slot);
	slot->romStart = romStart;
	slot->romEnd = romEnd;
	Node_Add(gSlotYazHead, slot);
}

void Dma_RegGroup(Rom* rom, DmaBank type) {
	switch (type) {
		case DMA_ACTOR:
			for (int i = 36; i <= 198; i++) {
				Dma_RegEntry(rom, i, 0x10);
			}
			for (int i = 235; i <= 496; i++) {
				Dma_RegEntry(rom, i, 0x10);
			}
			Dma_RegEntry(rom, 497, 0x1000);
			break;
		case DMA_STATE:
			for (int i = 29; i <= 32; i++) {
				Dma_RegEntry(rom, i, 0x10);
			}
			break;
		case DMA_KALEIDO:
			for (int i = 33; i <= 34; i++) {
				Dma_RegEntry(rom, i, 0x10);
			}
			break;
		case DMA_EFFECT:
			for (int i = 199; i <= 234; i++) {
				Dma_RegEntry(rom, i, 0x10);
			}
			break;
		case DMA_OBJECT:
			for (int i = 498; i <= 879; i++) {
				Dma_RegEntry(rom, i, 0x1000);
			}
			break;
		case DMA_PLACE_NAME:
			for (int i = 880; i <= 936; i++) {
				Dma_RegEntry(rom, i, 0x1000);
			}
			break;
		case DMA_SKYBOX_TEXEL:
			for (int i = 941; i <= 1004; i++) {
				Dma_RegEntry(rom, i, 0x1000);
				Dma_WriteFlag(i, false);
			}
			break;
		case DMA_SCENES:
			for (int i = 1007; i <= 1517; i++) {
				Dma_RegEntry(rom, i, 0x1000);
			}
			break;
		case DMA_UNUSED:
			for (int i = 1518; i < rom->table.num.dma; i++) {
				// Unused
				Dma_RegEntry(rom, i, 0x1000);
			}
			for (int i = rom->table.num.dma; i < rom->ext.dmaNum; i++) {
				gDma.entry[i].writable = true;
				gEntries++;
				gDma.highest = i;
			}
			
			Dma_RegSegment(rom, 0x35CF000, rom->file.size);
			break;
	}
}

void Dma_Free(void) {
	while (gSlotHead)
		Node_Kill(gSlotHead, gSlotHead);
	while (gSlotYazHead)
		Node_Kill(gSlotYazHead, gSlotYazHead);
	delete(gDma.entry);
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
			Node_Kill(gSlotHead, next);
			continue;
		}
		
		slot = slot->next;
	}
	
	if (g64.compress) {
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
				Node_Kill(gSlotYazHead, next);
				
				slot = gSlotYazHead;
				continue;
			}
			
			slot = slot->next;
		}
	}
}

void Dma_SlotInfo(Rom* rom, const char* message, Slot* head) {
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
	info("%s", message);
	printf("  " PRNT_GRAY "[  ");
	for (f32 i = 0; i < rom->file.size; i += MbToBin(1.00)) {
		u32 val = MbToBin(10.00);
		if (((u32)i % val) < 100) {
			int pos[2];
			
			cli_getPos(pos);
			printf("%.0f MB", BinToMb(i));
			cli_setPos(pos[0] + 1, pos[1]);
		} else {
			int pos[2];
			cli_getPos(pos);
			cli_setPos(pos[0] + 1, pos[1]);
		}
	}
	int pos[2];
	cli_getPos(pos);
	cli_setPos(pos[0] + 2, pos[1]);
	printf("" PRNT_GRAY "]\n");
	
	f32 i = 0;
	s32 c = 0;
	
	printf("  " PRNT_GRAY "[  ");
	while (slot != NULL) {
		s32 begn = 0;
		
		for (; i < rom->file.size; i += MbToBin(1.00)) {
			if (i >= slot->romStart && (i <= slot->romEnd || begn == 0)) {
				printf("" "%s" "=", color[c % ArrCount(color)]);
				begn = 1;
			} else if (i > slot->romEnd) {
				c++;
				break;
			} else {
				printf("" PRNT_GRAY "-");
			}
		}
		
		slot = slot->next;
	}
	printf("" PRNT_GRAY "  ]\n");
	info(gLang.rom.info_dma_entries, gEntries);
	info("%d", gDma.written);
	info_nl();
	
	int id = 0;
	slot = head;
	while (slot != NULL) {
		if (BinToMb(slot->romEnd - slot->romStart) < 1.0f)
			info(gLang.rom.info_dma_slot_kb, id++, slot->romStart, slot->romEnd, BinToKb(slot->romEnd - slot->romStart));
		else
			info(gLang.rom.info_dma_slot_mb, id++, slot->romStart, slot->romEnd, BinToMb(slot->romEnd - slot->romStart));
		
		slot = slot->next;
	}
	info_nl();
}

void Dma_WriteFlag(u32 id, bool value) {
	gDma.entry[id].writable = value;
}
