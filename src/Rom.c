#include "z64rom.h"

static u32 Overlay_GetBssSize(MemFile* dataFile) {
	u32* bssSize;
	
	SetSegment(0x1, dataFile->data);
	bssSize = SegmentedToVirtual(0x1, dataFile->dataSize - 4);
	bssSize = SegmentedToVirtual(0x1, dataFile->dataSize - ReadBE(bssSize[0]));
	
	return ReadBE(bssSize[3]);
}

static RestrictionFlag* Restriction_GetFlags(Rom* rom, u32 sceneIndex) {
	RestrictionFlag* flagList = rom->table.restrictionFlags;
	
	while (flagList->sceneIndex != 0xFF) {
		if (flagList->sceneIndex == sceneIndex)
			return flagList;
		
		flagList++;
	}
	
	return NULL;
}

static void Restriction_WriteFlags(Rom* rom, MemFile* config, u32 sceneIndex) {
	char* flags = Config_GetVariable(config->str, "restriction_flags");
	RestrictionFlag* rf = Restriction_GetFlags(rom, sceneIndex);
	
	if (flags == NULL || rf == NULL)
		return;
	
	memset(rf, 0, sizeof(RestrictionFlag));
	rf[0].sceneIndex = sceneIndex;
	
	if (StrStr(flags, "BOTTLES")) {
		rf->bottles = 3;
	}
	if (StrStr(flags, "A_BUTTON")) {
		rf->aButton = 3;
	}
	if (StrStr(flags, "B_BUTTON")) {
		rf->bButton = 3;
	}
	
	if (StrStr(flags, "WARP_SONG")) {
		rf->warpSong = 3;
	}
	if (StrStr(flags, "OCARINA")) {
		rf->ocarina = 3;
	}
	if (StrStr(flags, "HOOKSHOT")) {
		rf->hookshot = 3;
	}
	if (StrStr(flags, "TRADE_ITEM")) {
		rf->tradeItem = 3;
	}
	
	if (StrStr(flags, "ALL")) {
		rf->all = 3;
	}
	if (StrStr(flags, "DINS_FIRE")) {
		rf->din = 1;
	}
	if (StrStr(flags, "NAYRUS_LOVE")) {
		rf->nayry = 1;
	}
	if (StrStr(flags, "FARORES_WIND")) {
		rf->farore = 3;
	}
	if (StrStr(flags, "SUN_SONG")) {
		rf->sunSong = 3;
	}
}

static void ExtensionTable_Init(Rom* rom) {
	MemFile ulib = MemFile_Initialize();
	char* fname = "src/lib_user/uLib.h";
	char* word;
	
	if (MemFile_LoadFile_String(&ulib, fname))
		return;
	
	word = StrStr(ulib.str, "EXT_DMA_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_DMA_MAX", fname);
	rom->ext.dmaNum = Value_Int(Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_ACTOR_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_ACTOR_MAX", fname);
	rom->ext.actorNum = Value_Int(Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_OBJECT_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_OBJECT_MAX", fname);
	rom->ext.objectNum = Value_Int(Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_SCENE_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_SCENE_MAX", fname);
	rom->ext.sceneNum = Value_Int(Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_EFFECT_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_EFFECT_MAX", fname);
	rom->ext.effectNum = Value_Int(Word(word, 1));
	
	Log("DmaNum: %d", rom->ext.dmaNum);
	Log("ActorNum: %d", rom->ext.actorNum);
	Log("ObjectNum: %d", rom->ext.objectNum);
	Log("SceneNum: %d", rom->ext.sceneNum);
	Log("EffectNum: %d", rom->ext.effectNum);
	
	MemFile_Free(&ulib);
	
	Dma_FreeEntry(rom, DMA_ID_DMADATA, 0x10); Dma_WriteFlag(DMA_ID_DMADATA, false);
}

static void ExtensionTable_Alloc(Rom* rom) {
	#define Rom_MoveTable(OFFSET, O_TABLE, NUM, NEW_NUM) \
		OFFSET = rom->offset.table.dmaTable + size; \
		table = SegmentedToVirtual(0, OFFSET); \
		memcpy(table, O_TABLE, sizeof(*O_TABLE) * NUM); \
		O_TABLE = table; \
		NUM = NEW_NUM; \
		size += sizeof(*O_TABLE) * NEW_NUM; \
		size = Align(size, 16);
	
	u32 size = 0;
	void* table;
	
	if (rom->ext.dmaNum == 0)
		return;
	
	size += sizeof(struct DmaEntry) * rom->ext.dmaNum;
	size = Align(size, 16);
	
	Rom_MoveTable(rom->offset.table.actorTable, rom->table.actor, rom->table.num.actor, rom->ext.actorNum);
	Rom_MoveTable(rom->offset.table.objTable, rom->table.object, rom->table.num.obj, rom->ext.objectNum);
	Rom_MoveTable(rom->offset.table.sceneTable, rom->table.scene, rom->table.num.scene, rom->ext.sceneNum);
	Rom_MoveTable(rom->offset.table.effectTable, rom->table.effect, rom->table.num.effect, rom->ext.effectNum);
	
	if (rom->offset.table.dmaTable != Dma_AllocEntry(rom, 2, size))
		printf_error("Tables have turned!");
	
	for (s32 i = rom->table.num.dma; i < rom->ext.dmaNum; i++) {
		if (i == rom->ext.dmaNum - 1)
			rom->table.dma[i] = (DmaEntry) { 0 };
		
		else
			rom->table.dma[i] = (DmaEntry) { -1, -1, -1, -1 };
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

static void Config_WriteActor(MemFile* config, ActorEntry* actorOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteComment(config, name);
	Config_WriteHex(config, "vram_addr", ReadBE(actorOvl->vramStart), NO_COMMENT);
	Config_WriteHex(config, "init_vars", ReadBE(actorOvl->initInfo), NO_COMMENT);
	Config_WriteInt(config, "alloc_type", ReadBE(actorOvl->allocType), NO_COMMENT);
	MemFile_SaveFile_String(config, out);
}

static void Config_WriteEffect(MemFile* config, EffectEntry* actorOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteComment(config, name);
	Config_WriteHex(config, "vram_addr", ReadBE(actorOvl->vramStart), NO_COMMENT);
	Config_WriteHex(config, "init_vars", ReadBE(actorOvl->initInfo), NO_COMMENT);
	MemFile_SaveFile_String(config, out);
}

static void Config_WriteState(MemFile* config, GameStateEntry* stateOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteComment(config, name);
	Config_WriteHex(config, "vram_addr", ReadBE(stateOvl->vramStart), NO_COMMENT);
	Config_WriteHex(config, "init_func", ReadBE(stateOvl->init), NO_COMMENT);
	Config_WriteHex(config, "dest_func", ReadBE(stateOvl->destroy), NO_COMMENT);
	MemFile_SaveFile_String(config, out);
}

static void Config_WriteKaleido(Rom* rom, MemFile* config, u32 id, const char* name, char* out) {
	KaleidoEntry* entry = &rom->table.kaleido[id];
	u16* dataHi;
	u16* dataLo;
	u32 init;
	u32 dest;
	u32 updt;
	u32 draw;
	
	MemFile_Reset(config);
	Config_WriteComment(config, name);
	Config_WriteHex(config, "vram_addr", ReadBE(entry->vramStart), NO_COMMENT);
	
	if (id == 1) { // PLAYER
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.init.lo);
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.init.hi);
		init = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.dest.lo);
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.dest.hi);
		dest = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.updt.lo);
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.updt.hi);
		updt = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.draw.lo);
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.draw.hi);
		draw = (ReadBE(dataHi[1]) - ((s16)ReadBE(dataLo[1]) < 0)) << 16 | ReadBE(dataLo[1]);
		
		Config_WriteHex(config, "init", init, NO_COMMENT);
		Config_WriteHex(config, "dest", dest, NO_COMMENT);
		Config_WriteHex(config, "updt", updt, NO_COMMENT);
		Config_WriteHex(config, "draw", draw, NO_COMMENT);
	} else { // PAUSE_MENU
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.updt.hi);
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.updt.lo);
		init = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
		
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.draw.hi);
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.draw.lo);
		updt = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
		
		Config_WriteHex(config, "init", init, NO_COMMENT);
		Config_WriteHex(config, "updt", updt, NO_COMMENT);
	}
	
	MemFile_SaveFile_String(config, out);
}

static void Config_WriteScene(Rom* rom, MemFile* config, u32 id, const char* name, char* out) {
	#define FIWI \
		if (firstWritten > 0) { \
			MemFile_Printf(config, " | "); \
		} firstWritten++;
	
	SceneEntry* sceneEntry = &rom->table.scene[id];
	RestrictionFlag* rf = Restriction_GetFlags(rom, id);
	
	MemFile_Reset(config);
	Config_WriteComment(config, name);
#if 0
	Config_WriteInt(config, "unk_a", ReadBE(sceneEntry->unk_10), NO_COMMENT);
	Config_WriteInt(config, "unk_b", ReadBE(sceneEntry->unk_12), NO_COMMENT);
#endif
	Config_WriteInt(config, "scene_func_id", ReadBE(sceneEntry->config), NO_COMMENT);
	
	if (rf) {
		s32 firstWritten = 0;
		MemFile_Printf(config, "# [ BOTTLES / A_BUTTON / B_BUTTON / WARP_SONG / OCARINA / HOOKSHOT ]\n");
		MemFile_Printf(config, "# [ TRADE_ITEM / ALL / DINS_FIRE / NAYRUS_LOVE / FARORES_WIND / SUN_SONG ]\n");
		MemFile_Printf(config, "%-15s = \"", "restriction_flags");
		
		if (rf->bottles) {
			MemFile_Printf(config, "BOTTLES", "restriction_flags");
		}
		if (rf->aButton) {
			FIWI;
			MemFile_Printf(config, "A_BUTTON", "restriction_flags");
		}
		if (rf->bButton) {
			FIWI;
			MemFile_Printf(config, "B_BUTTON", "restriction_flags");
		}
		
		if (rf->warpSong) {
			FIWI;
			MemFile_Printf(config, "WARP_SONG", "restriction_flags");
		}
		if (rf->ocarina) {
			FIWI;
			MemFile_Printf(config, "OCARINA", "restriction_flags");
		}
		if (rf->hookshot) {
			FIWI;
			MemFile_Printf(config, "HOOKSHOT", "restriction_flags");
		}
		if (rf->tradeItem) {
			FIWI;
			MemFile_Printf(config, "TRADE_ITEM", "restriction_flags");
		}
		
		if (rf->all) {
			FIWI;
			MemFile_Printf(config, "ALL", "restriction_flags");
		}
		if (rf->din) {
			FIWI;
			MemFile_Printf(config, "DINS_FIRE", "restriction_flags");
		}
		if (rf->nayry) {
			FIWI;
			MemFile_Printf(config, "NAYRUS_LOVE", "restriction_flags");
		}
		if (rf->farore) {
			FIWI;
			MemFile_Printf(config, "FARORES_WIND", "restriction_flags");
		}
		if (rf->sunSong) {
			FIWI;
			MemFile_Printf(config, "SUN_SONG", "restriction_flags");
		}
		
		if (firstWritten == 0) {
			MemFile_Printf(config, "nothing\"\n");
		} else {
			MemFile_Printf(config, "\"\n");
		}
	}
	
	MemFile_SaveFile_String(config, out);
#undef FIWI
}

// # # # # # # # # # # # # # # # # # # # #
// # PATCH                               #
// # # # # # # # # # # # # # # # # # # # #

Patch gPatch;

static void Patch_Init() {
	ItemList list = ItemList_Initialize();
	
	ItemList_SetFilter(&list, CONTAIN_END, ".cfg");
	ItemList_List(&list, "patch/", -1, LIST_FILES | LIST_NO_DOT);
	Calloc(gPatch.cfg.file, sizeof(struct MemFile) * list.num);
	
	for (s32 i = 0; i < list.num; i++) {
		MemFile* mem = &gPatch.cfg.file[gPatch.cfg.num++];
		char* line;
		u32 lineCount;
		
		MemFile_LoadFile_String(mem, list.item[i]);
		MemFile_Realloc(mem, mem->memSize * 8);
		Config_ProcessIncludes(mem);
		Log("Processed");
		
		// @macro
		line = mem->str;
		lineCount = LineNum(mem->str);
		for (s32 i = 0; i < lineCount; i++, line = Line(line, 1)) {
			if (line == NULL)
				break;
			if (line[0] != '@')
				continue;
			
			char* cmd = CopyWord(line, 0);
			
			StrRepWhole(mem->str, cmd + 1, Config_GetVariable(line, cmd));
			line[0] = '#';
		}
	}
	
	ItemList_Free(&list);
	ItemList_List(&list, "rom/lib_code/", -1, LIST_FILES | LIST_NO_DOT);
	Calloc(gPatch.bin.file, sizeof(struct MemFile) * list.num);
	Calloc(gPatch.bin.offset, sizeof(u32) * list.num);
	
	for (s32 i = 0; i < list.num; i++) {
		if (StrEndCase(list.item[i], ".bin")) {
			u32* offset = &gPatch.bin.offset[gPatch.bin.num];
			MemFile* mem = &gPatch.bin.file[gPatch.bin.num++];
			MemFile cfg;
			char* tname = xAlloc(strlen(list.item[i] + 8));
			
			strcpy(tname, list.item[i]);
			StrRep(tname, ".bin", ".cfg");
			
			if (MemFile_LoadFile(mem, list.item[i])) printf_error("Could not open [%s]", list.item[i]);
			if (MemFile_LoadFile_String(&cfg, tname)) printf_error("Could not open [%s]", tname);
			*offset = Config_GetInt(&cfg, "rom");
			
			if (Config_Variable(cfg.str, "next") &&
				Config_GetInt(&cfg, "next") - Config_GetInt(&cfg, "ram") < mem->dataSize)
				printf_error("Can't fit [%s] between z64ram - z64next!", list.item[i]);
			
			MemFile_Free(&cfg);
		}
	}
	
	ItemList_Free(&list);
}

static void Patch_Free() {
	for (s32 i = 0; i < gPatch.cfg.num; i++)
		MemFile_Free(&gPatch.cfg.file[i]);
	Free(gPatch.cfg.file);
	
	for (s32 i = 0; i < gPatch.bin.num; i++)
		MemFile_Free(&gPatch.bin.file[i]);
	Free(gPatch.bin.file);
	Free(gPatch.bin.offset);
}

static s32 Patch_File(MemFile* memDest, const char* section) {
	PatchNode* nodeHead = NULL;
	u32 isPatched = false;
	
	if (section) {
		section = qFree(StrDup(section));
		
		StrRep((char*)section, gVanilla, "");
		StrRep((char*)section, "//", "/");
	}
	
	for (s32 p = 0; p < gPatch.cfg.num; p++) {
		ItemList vlist;
		MemFile* cfg = &gPatch.cfg.file[p];
		u32 reloc = 0;
		
		if (section && !StrStr(cfg->str, section))
			continue;
		
		Config_GotoSection(section);
		Config_ListVariables(cfg, &vlist, section);
		
		forlist(i, vlist) {
			char* variable;
			char* temp;
			s64 addr;
			u32 isHex = true;
			
			if (!strcmp(vlist.item[i], "reloc_from")) {
				reloc = Config_GetInt(cfg, vlist.item[i]);
				
				// Comment out already processed variables
				LineHead(
					Config_Variable(cfg->str, vlist.item[i]),
					cfg->str
				)[0] = '#';
				
				continue;
			}
			
			if (!Value_ValidateHex(vlist.item[i]))
				continue;
			
			addr = Value_Hex(vlist.item[i]) - reloc;
			variable = Config_GetVariable(cfg->str, vlist.item[i]);
			
			isPatched = 1;
			temp = Config_Variable(cfg->str, vlist.item[i]);
			
			if (temp[0] == '\"' || !memcmp(temp, "FILE(", 5))
				isHex = false;
			
			// Comment out already processed variables
			LineHead(temp, cfg->str)[0] = '#';
			
			if (addr + strlen(variable) >= memDest->dataSize || addr < 0) {
				printf_warning("\aPatch [0x%08X] from [%s] does not fit into [%s]", addr, cfg->info.name, section);
				continue;
			}
			
			PatchNode* node = xAlloc(sizeof(struct PatchNode));
			
			Node_Add(nodeHead, node);
			node->start = addr + reloc;
			strcpy(node->source, cfg->info.name);
			if (section)
				strcpy(node->section, section);
			
			if (isHex) {
				u32 len = strlen(variable);
				u32 wp = 0;
				
				for (s32 j = 0; j < len; j++) {
					u8 val;
					u8* dst = &memDest->cast.u8[addr + (s32)floorf((f32)wp / 2)];
					
					if (!memcmp(&variable[j], "0x", 2))
						continue;
					
					if (variable[j] == 'x' || variable[j] == ' ' || variable[j] == '\t')
						continue;
					
					if (variable[j] == '.') {
						wp++;
						continue;
					}
					
					val = Value_Hex(xFmt("%c", variable[j]));
					
					if (wp % 2 == 0) {
						*dst &= ~0xF0;
						*dst |= val << 4;
					} else {
						*dst &= ~0x0F;
						*dst |= val & 0xF;
					}
					wp++;
				}
				
				node->end = addr + wp + reloc;
			} else {
				MemFile_Seek(memDest, addr);
				
				if (!memcmp(variable, "ILE(", 4)) {
					MemFile bmem;
					char* bin = StrDup(variable);
					
					StrRep(bin, "ILE(\"", "");
					StrRep(bin, ")", "");
					
					FileSys_Path(Path(cfg->info.name));
					
					if (!Sys_Stat(FileSys_File(bin))) {
						printf_warning("Could not locate file [%s] referenced by patch [%s]", FileSys_File(bin), cfg->info.name);
						
						continue;
					}
					
					MemFile_LoadFile(&bmem, FileSys_File(bin));
					node->end = addr + bmem.dataSize + reloc;
					MemFile_Append(memDest, &bmem);
					MemFile_Free(&bmem);
				} else {
					MemFile_Write(memDest, variable, strlen(variable));
					node->end = addr + strlen(variable) + reloc;
				}
			}
		}
		
		ItemList_Free(&vlist);
	}
	
	Config_GotoSection(NULL);
	
	if (!StrStr(section, "z_code.bin") && !StrStr(section, "z_boot.bin"))
		return isPatched;
	
	u32 reloc;
	u32 end;
	
	if (StrStr(section, "z_boot.bin")) {
		reloc = RELOC_BOOT;
		end = 0x00012F70;
	} else {
		reloc = RELOC_CODE;
		end = 0x00BCEF30;
	}
	
	for (s32 p = 0; p < gPatch.bin.num; p++) {
		u32 offset = gPatch.bin.offset[p];
		
		if (offset >= reloc && offset <= end) {
			PatchNode* node = nodeHead;
			MemFile_Seek(memDest, offset - reloc);
			MemFile_Append(memDest, &gPatch.bin.file[p]);
			isPatched = 1;
			
			while (node) {
				if (Intersect(node->start, node->end, offset, offset + gPatch.bin.file[p].dataSize))
					printf_warning(
						"" PRNT_YELW "WARNING!" PRNT_RSET
						"\nCfg patch from [" PRNT_YELW "%s" PRNT_RSET "] to [" PRNT_REDD "%s" PRNT_GRAY ":" PRNT_REDD "%X" PRNT_RSET "]"
						"\nhas been overwritten by a binary patch [" PRNT_YELW "%s" PRNT_RSET "]",
						node->source,
						strlen(node->section) ? node->section : "rom",
						node->start,
						gPatch.bin.file[p].info.name
					);
				
				node = node->next;
			}
		}
	}
	
	return isPatched;
}

// # # # # # # # # # # # # # # # # # # # #
// # Dump                                #
// # # # # # # # # # # # # # # # # # # # #

static Size sBaseromSize;

typedef enum {
	/*  0 */ TRANS_TYPE_WIPE,
	/*  1 */ TRANS_TYPE_TRIFORCE,
	/*  2 */ TRANS_TYPE_FADE_BLACK,
	/*  3 */ TRANS_TYPE_FADE_WHITE,
	/*  4 */ TRANS_TYPE_FADE_BLACK_FAST,
	/*  5 */ TRANS_TYPE_FADE_WHITE_FAST,
	/*  6 */ TRANS_TYPE_FADE_BLACK_SLOW,
	/*  7 */ TRANS_TYPE_FADE_WHITE_SLOW,
	/*  8 */ TRANS_TYPE_WIPE_FAST,
	/*  9 */ TRANS_TYPE_FILL_WHITE2,
	/* 10 */ TRANS_TYPE_FILL_WHITE,
	/* 11 */ TRANS_TYPE_INSTANT,
	/* 12 */ TRANS_TYPE_FILL_BROWN,
	/* 13 */ TRANS_TYPE_FADE_WHITE_CS_DELAYED,
	/* 14 */ TRANS_TYPE_SANDSTORM_PERSIST,
	/* 15 */ TRANS_TYPE_SANDSTORM_END,
	/* 16 */ TRANS_TYPE_CS_BLACK_FILL,
	/* 17 */ TRANS_TYPE_FADE_WHITE_INSTANT,
	/* 18 */ TRANS_TYPE_FADE_GREEN,
	/* 19 */ TRANS_TYPE_FADE_BLUE,
	
	TRANS_TYPE_CIRCLE = 0x20,
	TRANS_TYPE_WARP   = 0x2C,
	
	TRANS_TYPE_MAX    = 56,
} TransitionType;

const char* sTransName[] = {
	/*  0 */ "Wipe",
	/*  1 */ "Triforce",
	/*  2 */ "FadeBlack",
	/*  3 */ "FadeWhite",
	/*  4 */ "FadeBlackFast",
	/*  5 */ "FadeWhiteFast",
	/*  6 */ "FadeBlackSlow",
	/*  7 */ "FadeWhiteSlow",
	/*  8 */ "WipeFast",
	/*  9 */ "FillWhite2",
	/* 10 */ "FillWhite",
	/* 11 */ "Instant",
	/* 12 */ "FillBrown",
	/* 13 */ "FadeWhiteCsDelayed",
	/* 14 */ "Sandstorm",
	/* 15 */ "SandstormEnd",
	/* 16 */ "FillBlack",
	/* 17 */ "FadeWhiteInstant",
	/* 18 */ "FadeGreen",
	/* 19 */ "FadeBlue",
	/* 20 */ "",
	/* 21 */ "",
	/* 22 */ "",
	/* 23 */ "",
	/* 24 */ "",
	/* 25 */ "",
	/* 26 */ "",
	/* 27 */ "",
	/* 28 */ "",
	/* 29 */ "",
	/* 30 */ "",
	/* 31 */ "",
	/* 32 */ "Circle",
	/* 33 */ "",
	/* 34 */ "",
	/* 35 */ "",
	/* 36 */ "",
	/* 37 */ "",
	/* 38 */ "",
	/* 39 */ "",
	/* 40 */ "",
	/* 41 */ "",
	/* 42 */ "",
	/* 43 */ "",
	/* 44 */ "Warp",
};

static const char* Transition_GetName(TransitionType type) {
	if (type >= TRANS_TYPE_MAX)
		printf_error("WOT?! %d", type);
	
	if (type >= TRANS_TYPE_CIRCLE)
		type = TRANS_TYPE_CIRCLE;
	
	if (strlen(sTransName[type]) == 0)
		printf_error("ID [%d] not valid?", type);
	
	return sTransName[type];
}

static TransitionType Transition_GetType(const char* str) {
	TransitionType i = 0;
	
	for (;; i++) {
		if (!strcmp(str, sTransName[i])) {
			return i;
		}
		
		if (i >= TRANS_TYPE_MAX)
			printf_error("'%s' is not actual transition type...", str);
	}
	
	printf_error("Could not solve id for transition type [%s]", str);
	
	return 0;
}

static void Dump_Actor(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < rom->table.num.actor; i++) {
		rf = Dma_RomFile_Actor(rom, i);
		
		if (rf.size == 0)
			continue;
		
		printf_progress("Actor", i + 1, rom->table.num.actor);
		FileSys_Path("rom/actor/%s/0x%04X-%s/", gVanilla, i, gActorName_OoT[i]);
		
		if (Rom_Extract(data, rf, FileSys_File("actor.zovl")))
			Config_WriteActor(config, &rom->table.actor[i], gActorName_OoT[i], FileSys_File("config.cfg"));
	}
}

static void Dump_Effect(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < rom->table.num.effect; i++) {
		rf = Dma_RomFile_Effect(rom, i);
		
		if (rf.size == 0)
			continue;
		
		printf_progress("Effect", i + 1, rom->table.num.effect);
		FileSys_Path("rom/effect/%s/0x%04X-%s/", gVanilla, i, gEffectName_OoT[i]);
		
		if (Rom_Extract(data, rf, FileSys_File("effect.zovl")))
			Config_WriteEffect(config, &rom->table.effect[i], gEffectName_OoT[i], FileSys_File("config.cfg"));
	}
}

static void Dump_Object(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < rom->table.num.obj; i++) {
		rf = Dma_RomFile_Object(rom, i);
		
		if (rf.size == 0)
			continue;
		
		printf_progress("Object", i + 1, rom->table.num.obj);
		FileSys_Path("rom/object/%s/0x%04X-%s/", gVanilla, i, gObjectName_OoT[i]);
		
		Rom_Extract(data, rf, FileSys_File("object.zobj"));
		
		if (i != 0x1)
			continue;
		
		SetSegment(1, rf.data);
		PlayerAnimEntry* entry = (PlayerAnimEntry*)SegmentedToVirtual(1, 0x2310);
		char* anim = SegmentedToVirtual(0, 0x4E5C00);
		
		Sys_MakeDir("rom/system/animation/%s/", gVanilla);
		
		for (s32 j = 0; entry[j].__pad == 0; j++) {
			u32 segment = entry[j].segment & 0xFFFFFF;
			const char* file = gPlayerAnimName[j];
			
			if (segment == 0)
				continue;
			
			rf.data = anim + segment;
			rf.size = sizeof(PlayerAnimFrame) * entry[j].frameCount;
			
			if (!file)
				file = "Anim";
			
			Rom_Extract(data, rf, xFmt("rom/system/animation/%s/%d-%s.bin", gVanilla, j, file));
		}
	}
}

static void Dump_Scene(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < rom->table.num.scene; i++) {
		SceneEntry* scene = &rom->table.scene[i];
		RomFile png;
		rf = Dma_RomFile_Scene(rom, i);
		
		if (rf.size == 0)
			continue;
		
		printf_progress("Scene", i + 1, rom->table.num.scene);
		FileSys_Path("rom/scene/%s/0x%02X-%s/", gVanilla, i, gSceneName_OoT[i]);
		
		if (Rom_Extract(data, rf, FileSys_File("scene.zscene"))) {
			u32* seg;
			u32 roomNum;
			u32 roomListSeg;
			u32* vromSeg;
			
			Config_WriteScene(rom, config, i, gSceneName_OoT[i], FileSys_File("config.cfg"));
			SetSegment(0x2, rf.data);
			seg = data->data;
			
			while (1) {
				if ((seg[0] & 0xFF) == 0x04) {
					break;
				}
				if ((seg[0] & 0xFF) == 0x14) {
					printf_warning_align("Scene", "Failed finding room list");
					seg = 0;
					break;
				}
				seg++;
			}
			
			if (seg) {
				roomNum = (seg[0] & 0xFF00) >> 8;
				roomListSeg = ReadBE(seg[1]) & 0xFFFFFF;
				
				for (s32 j = 0; j < roomNum; j++) {
					char* out = FileSys_File(xFmt("room_%d.zroom", j));
					
					vromSeg = SegmentedToVirtual(0x2, roomListSeg + 8 * j);
					Rom_Extract(
						data,
						Rom_GetRomFile(rom, vromSeg[0], vromSeg[1]),
						out
					);
				}
			}
		}
		
		png.romStart = ReadBE(scene->titleVromStart);
		png.romEnd = ReadBE(scene->titleVromEnd);
		png.data = SegmentedToVirtual(0, png.romStart);
		png.size = png.romEnd - png.romStart;
		
		if (png.romStart != 0 && png.size > 0) {
			Texel_Dump(
				&png,
				FileSys_File("title.png"),
				TEX_FMT_IA,
				TEX_BSIZE_8,
				144,
				24
			);
		}
	}
}

static void Dump_State(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < rom->table.num.state; i++) {
		rf = Dma_RomFile_GameState(rom, i);
		
		if (rf.size == 0)
			continue;
		
		printf_progress("System", i + 1, rom->table.num.state);
		FileSys_Path("rom/system/state/%s/0x%02X-%s/", gVanilla, i, gStateName_OoT[i]);
		
		if (Rom_Extract(data, rf, FileSys_File("state.zovl")))
			Config_WriteState(config, &rom->table.state[i], gStateName_OoT[i], FileSys_File("config.cfg"));
	}
}

static void Dump_Kaleido(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < rom->table.num.kaleido; i++) {
		FileSys_Path("rom/system/kaleido/%s/0x%02X-%s/", gVanilla, i, gKaleidoName_OoT[i]);
		
		rf.size = ReadBE(rom->table.kaleido[i].vromEnd) - ReadBE(rom->table.kaleido[i].vromStart);
		rf.data = SegmentedToVirtual(0x0, ReadBE(rom->table.kaleido[i].vromStart));
		
		Rom_Extract(data, rf, FileSys_File("overlay.zovl"));
		Config_WriteKaleido(rom, config, i, gKaleidoName_OoT[i], FileSys_File("config.cfg"));
	}
}

static void Dump_Skybox(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	for (s32 i = 0; i < 32; i++) {
		printf_progress("Skybox", i + 1, 32);
		
		FileSys_Path("rom/system/skybox/%s/0x%02X-%s/", gVanilla, i, gSkyboxName_OoT[i]);
		
		rf.romStart = ReadBE(rom->table.dma[941 + i * 2].vromStart);
		rf.romEnd = ReadBE(rom->table.dma[941 + i * 2].vromEnd);
		rf.size = rf.romEnd - rf.romStart;
		rf.data = SegmentedToVirtual(0x0, rf.romStart);
		Rom_Extract(data, rf, FileSys_File("skybox.tex"));
		
		rf.romStart = ReadBE(rom->table.dma[942 + i * 2].vromStart);
		rf.romEnd = ReadBE(rom->table.dma[942 + i * 2].vromEnd);
		rf.size = rf.romEnd - rf.romStart;
		rf.data = SegmentedToVirtual(0x0, rf.romStart);
		Rom_Extract(data, rf, FileSys_File("skybox.pal"));
	}
}

static void Dump_Static(Rom* rom, MemFile* data, MemFile* config) {
	RomFile rf;
	
	foreach(i, gSystem_OoT) {
		u32 id = gSystem_OoT[i].id;
		MessageTableEntry* tbl = NULL;
		
		printf_progress("Static", i + 1, ArrayCount(gSystem_OoT));
		FileSys_Path("rom/system/static/%s/", gVanilla);
		
		rf.romStart = ReadBE(rom->table.dma[id].vromStart);
		rf.romEnd = ReadBE(rom->table.dma[id].vromEnd);
		rf.size = rf.romEnd - rf.romStart;
		rf.data = SegmentedToVirtual(0x0, rf.romStart);
		
		switch (id) {
			case DMA_ID_DO_ACTION_STATIC:
				rf.size = 0x2B80;
				break;
			case DMA_ID_ITEM_NAME_STATIC:
				rf.size = 0x1EC00;
				break;
			case DMA_ID_MESSAGE_DATA_STATIC_NES:
				tbl = rom->table.nesMsg;
				break;
			case DMA_ID_MESSAGE_DATA_STATIC_STAFF:
				tbl = rom->table.staffMsg;
				break;
		}
		
		if (tbl) {
			RomFile tblRf = { 0 };
			
			tblRf.data = tbl;
			
			for (s32 i = 0;; i++) {
				tblRf.size += sizeof(MessageTableEntry);
				if (tbl[i].textId == 0xFFFF)
					break;
			}
			
			Rom_Extract(data, tblRf, FileSys_File(xFmt("%s.tbl", gSystem_OoT[i].name)));
		}
		
		Rom_Extract(data, rf, FileSys_File(xFmt("%s.bin", gSystem_OoT[i].name)));
	}
}

static void Dump_EntranceTable(Rom* rom, MemFile* memData, MemFile* memCfg) {
	EntranceInfo* ent = rom->table.entrance;
	
	MemFile_Reset(memData);
	
	MemFile_Printf(memData, "# Transition Types:\n");
	foreach(i, sTransName) {
		if (strlen(sTransName[i]))
			MemFile_Printf(memData, "\t# %s\n", sTransName[i]);
	}
	MemFile_Printf(memData, "\n");
	
	MemFile_Printf(memData, "# Array Items: [ scene_id, spawn_id, continue_bgm, title_card, fade_in, fade_out ]\n\n");
	
	for (s32 i = 0; i < rom->table.num.entrance; i++, ent++) {
		ItemList list = ItemList_Initialize();
		
		if (i == 0 || (ent->scene != ent[-1].scene || ent->spawn != ent[-1].spawn))
			MemFile_Printf(memData, "# %s \n", gSceneName_OoT[ent->scene]);
		
		ItemList_Alloc(&list, 6, 0x80);
		ItemList_AddItem(&list, xFmt("0x%02X", ent->scene));
		ItemList_AddItem(&list, xFmt("0x%02X", ent->spawn));
		ItemList_AddItem(&list, xFmt("%s", ent->continueBgm ? "true" : "false"));
		ItemList_AddItem(&list, xFmt("%s", ent->titleCard ? "true" : "false"));
		ItemList_AddItem(&list, xFmt("\"%s\"", Transition_GetName(ent->fadeIn)));
		ItemList_AddItem(&list, xFmt("\"%s\"", Transition_GetName(ent->fadeOut)));
		
		Config_WriteArray(memData, xFmt("0x%04X", i), &list, NO_QUOTES, NO_COMMENT);
		
		ItemList_Free(&list);
	}
	
	MemFile_SaveFile_String(memData, "rom/system/entrance_table.cfg");
	MemFile_SaveFile_String(memData, "rom/system/backup.entrance_table.cfg");
}

// # # # # # # # # # # # # # # # # # # # #
// # Build                               #
// # # # # # # # # # # # # # # # # # # # #

static void Build_Actor(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	ActorEntry* entry = rom->table.actor;
	
	Rom_ItemList(&list, "rom/actor/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (s32 i = 0; i < list.num; i++) {
		if (list.item[i] == NULL) {
			if (entry[i].vromStart && entry[i].vromEnd)
				entry[i] = (ActorEntry) { 0 };
			continue;
		}
		
		if (i >= rom->table.num.actor) {
			printf_warning("ActorTable Full [%d/%d]", i + 1, rom->table.num.actor);
			break;
		} else
			printf_progress("Actor", i + 1, list.num);
		
		FileSys_Path(list.item[i]);
		
		MemFile_Reset(memData);
		MemFile_Reset(memCfg);
		MemFile_LoadFile(memData, FileSys_FindFile(".zovl"));
		MemFile_LoadFile_String(memCfg, FileSys_File("config.cfg"));
		
		entry[i].allocType = Config_GetInt(memCfg, "alloc_type");
		entry[i].initInfo = Config_GetInt(memCfg, "init_vars");
		
		entry[i].vramStart = Config_GetInt(memCfg, "vram_addr");
		entry[i].vramEnd = entry[i].vramStart + memData->dataSize + Overlay_GetBssSize(memData);
		
		s32 p = Patch_File(memData, memData->info.name);
		entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, memData, p ? NOCACHE_COMPRESS : COMPRESS);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].allocType);
		SwapBE(entry[i].initInfo);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		
		entry[i].loadedRamAddr = 0;
		
		if (gBuildTarget == ROM_DEV && entry[i].name) {
			u32 romAddr = ReadBE(entry[i].name) - 0x7F588E60;
			void* target;
			char buf[64];
			u32 id;
			
			Log("Filename Offset [%08X]", romAddr);
			
			target = SegmentedToVirtual(SEG_CODE, romAddr - RELOC_CODE);
			sscanf(PathSlot(list.item[i], -1), "0x%04X-%s/", &id, buf);
			strncpy(target, buf, strlen(target));
		}
	}
	
	ItemList_Free(&list);
}

static void Build_Effect(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	EffectEntry* entry = rom->table.effect;
	
	Rom_ItemList(&list, "rom/effect/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (s32 i = 0; i < list.num; i++) {
		if (list.item[i] == NULL) {
			entry[i] = (EffectEntry) { 0 };
			continue;
		}
		
		if (i >= rom->table.num.effect) {
			printf_warning("Illegal action! Can't have more than " PRNT_REDD "0x%X" PRNT_RSET " effects!", rom->table.num.effect);
			break;
		} else
			printf_progress("Effect", i + 1, list.num);
		
		FileSys_Path(list.item[i]);
		
		MemFile_Reset(memData);
		MemFile_Reset(memCfg);
		MemFile_LoadFile(memData, FileSys_FindFile(".zovl"));
		MemFile_LoadFile_String(memCfg, FileSys_File("config.cfg"));
		
		entry[i].initInfo = Config_GetInt(memCfg, "init_vars");
		
		entry[i].vramStart = Config_GetInt(memCfg, "vram_addr");
		entry[i].vramEnd = entry[i].vramStart + memData->dataSize + Overlay_GetBssSize(memData);
		
		entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, memData, true);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].initInfo);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		entry[i].loadedRamAddr = 0;
	}
	
	ItemList_Free(&list);
}

static void Build_Object(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	ObjectEntry* entry = rom->table.object;
	
	Rom_ItemList(&list, "rom/object/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (s32 i = 0; i < list.num; i++) {
		s32 compress = true;
		
		if (list.item[i] == NULL) {
			entry[i].vromStart = 0;
			entry[i].vromEnd = 0;
			
			continue;
		}
		
		if (i >= rom->table.num.obj) {
			printf_warning("Illegal action! Can't have more than " PRNT_REDD "0x%X" PRNT_RSET " objects!", rom->table.num.obj);
			break;
		} else
			printf_progress("Object", i + 1, list.num);
		
		FileSys_Path(list.item[i]);
		
		MemFile_Reset(memData);
		MemFile_LoadFile(memData, FileSys_FindFile(".zobj"));
		
		if (i == 1) {
			ItemList animList = ItemList_Initialize();
			PlayerAnimEntry* animEntry;
			
			Rom_ItemList(&animList, "rom/system/animation/", SORT_NUMERICAL, LIST_FILES);
			SetSegment(1, memData->data);
			animEntry = (PlayerAnimEntry*)SegmentedToVirtual(1, 0x2310);
			
			forlist(j, animList) {
				MemFile mem;
				
				if (animList.item[j] == NULL)
					continue;
				
				MemFile_LoadFile(&mem, animList.item[j]);
				
				animEntry[j].segment = rom->playerAnim.seekPoint | 0x07000000;
				animEntry[j].frameCount = mem.dataSize / sizeof(PlayerAnimFrame);
				
				MemFile_Append(&rom->playerAnim, &mem);
				MemFile_Align(&rom->playerAnim, 0x2);
				
				MemFile_Free(&mem);
			}
			
			ItemList_Free(&animList);
			compress = NOCACHE_COMPRESS;
		}
		
		entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, memData, compress);
		entry[i].vromEnd = Dma_GetVRomEnd();
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
	}
	
	ItemList_Free(&list);
}

static void Build_Scene(Rom* rom, MemFile* memData, MemFile* memCfg) {
	MemFile memRoom = MemFile_Initialize();
	ItemList list = ItemList_Initialize();
	ItemList titleList = ItemList_Initialize();
	SceneEntry* entry = rom->table.scene;
	u32* titleID;
	
	MemFile_Alloc(&memRoom, MbToBin(2));
	MemFile_Params(&memRoom, MEM_REALLOC, true, MEM_END);
	Rom_ItemList(&list, "rom/scene/", SORT_NUMERICAL, LIST_FOLDERS);
	
	ItemList_Alloc(&titleList, list.num, list.writePoint * 4);
	Calloc(titleID, sizeof(u32) * list.num);
	
	for (s32 i = 0; i < list.num; i++) {
		u32* seg;
		u32* vromSeg;
		u32 roomNum;
		u32 roomListSeg;
		u8* preDigest;
		u8* postDigest;
		char* zscene;
		
		if (list.item[i] == NULL)
			printf_error("Empty scene %d", i);
		
		if (i >= rom->table.num.scene) {
			printf_warning("Illegal action! Can't have more than " PRNT_REDD "0x%X" PRNT_RSET " scenes!", rom->table.num.scene);
			break;
		} else
			printf_progress("Scene", i + 1, list.num);
		
		FileSys_Path(list.item[i]);
		
		zscene = FileSys_FindFile(".zscene");
		
		MemFile_Reset(memCfg);
		MemFile_Reset(memData);
		MemFile_LoadFile_String(memCfg, FileSys_File("config.cfg"));
		MemFile_LoadFile(memData, zscene);
		
		preDigest = Sys_Sha256(memData->data, memData->dataSize);
		
		SetSegment(0x1, memData->data);
		seg = SegmentedToVirtual(0x1, 0);
		
		Restriction_WriteFlags(rom, memCfg, i);
		
		for (;;) {
			if ((seg[0] & 0xFF) == 0x04) {
				break;
			}
			if ((seg[0] & 0xFF) == 0x14) {
				printf_warning_align("Scene", "Failed finding room list");
				seg = 0;
				break;
			}
			seg++;
		}
		
		roomNum = (seg[0] & 0xFF00) >> 8;
		roomListSeg = ReadBE(seg[1]) & 0xFFFFFF;
		vromSeg = SegmentedToVirtual(0x1, roomListSeg);
		
		for (s32 j = 0; j < roomNum; j++) {
			u32 id = j * 2;
			char* room;
			
			MemFile_Reset(&memRoom);
			room = FileSys_File(xFmt("room_%d.zroom", j));
			if (!Sys_Stat(room)) room = FileSys_File(xFmt("room_%d.zmap", j));
			if (!Sys_Stat(room)) printf_error("Could not find room_%d", j);
			MemFile_LoadFile(&memRoom, room);
			
			vromSeg[id + 0] = Dma_WriteEntry(rom, DMA_FIND_FREE, &memRoom, true);
			vromSeg[id + 1] = Dma_GetVRomEnd();
			SwapBE(vromSeg[id + 0]);
			SwapBE(vromSeg[id + 1]);
		}
		
		u32* hdr = SegmentedToVirtual(0x1, 0);
		for (;; hdr++) {
			if ((hdr[0] & 0xFF) == 0x18) {
				break;
			}
			if ((hdr[0] & 0xFF) == 0x14) {
				hdr = NULL;
				break;
			}
		}
		
		if (hdr) {
			u32 num;
			u32* room;
			room = hdr = SegmentedToVirtual(0x1, ReadBE(hdr[1]) & 0x00FFFFFF);
			for (s32 r = 0;; r++) {
				if ((hdr[r] & 0xFF) != 0x2 && hdr[r] != 0)
					break;
				room = SegmentedToVirtual(0x1, ReadBE(hdr[r]) & 0xFFFFFF);
				for (;; room++) {
					if ((room[0] & 0xFF) == 0x04) {
						break;
					}
					if ((room[0] & 0xFF) == 0x14) {
						room = NULL;
						break;
					}
				}
				
				if (room) {
					u32 seg;
					num = (room[0] & 0xFF00) >> 8;
					seg = ReadBE(room[1]) & 0xFFFFFF;
					room = SegmentedToVirtual(0x1, seg);
					
					for (s32 j = 0; j < num; j++) {
						u32 id = j * 2;
						
						room[id + 0] = vromSeg[id + 0];
						room[id + 1] = vromSeg[id + 1];
					}
				}
			}
		}
		
		postDigest = Sys_Sha256(memData->data, memData->dataSize);
		
		if (memcmp(preDigest, postDigest, memData->dataSize))
			MemFile_SaveFile(memData, zscene);
		
		Free(preDigest);
		Free(postDigest);
		
		entry[i].config = Config_GetInt(memCfg, "scene_func_id");
		entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, memData, true);
		entry[i].vromEnd = Dma_GetVRomEnd();
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		entry[i].titleVromStart = 0;
		entry[i].titleVromEnd = 0;
		
		char* f = FileSys_File("title.png");
		
		if (Sys_Stat(f)) {
			titleID[titleList.num] = i;
			ItemList_AddItem(&titleList, f);
		}
	}
	
	// Check for unique Place Names
	for (s32 i = 0; i < titleList.num; i++) {
		u32 useSame = false;
		Texel_Import(memData, titleList.item[i], TEX_FMT_IA, TEX_BSIZE_8, 144, 24);
		
		for (s32 j = 0; j < i; j++) {
			u32 id = titleID[j];
			void* data;
			Size sz;
			
			data = SegmentedToVirtual(0, ReadBE(entry[id].titleVromStart));
			sz = ReadBE(entry[id].titleVromEnd) - ReadBE(entry[id].titleVromStart);
			
			if (!memcmp(data, memData->data, sz)) {
				useSame = true;
				
				entry[titleID[i]].titleVromStart = entry[id].titleVromStart;
				entry[titleID[i]].titleVromEnd = entry[id].titleVromEnd;
			}
		}
		
		if (useSame == false) {
			entry[titleID[i]].titleVromStart = ReadBE(Dma_WriteEntry(rom, DMA_FIND_FREE, memData, true));
			entry[titleID[i]].titleVromEnd = ReadBE(Dma_GetVRomEnd());
		}
	}
	
	MemFile_Free(&memRoom);
	ItemList_Free(&list);
	ItemList_Free(&titleList);
	Free(titleID);
}

static void Build_State(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	GameStateEntry* entry = rom->table.state;
	
	Rom_ItemList(&list, "rom/system/state/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (s32 i = 0; i < list.num; i++) {
		printf_progress("System", i + 1, list.num);
		
		if (list.item[i] == NULL)
			continue;
		
		FileSys_Path(list.item[i]);
		
		MemFile_Reset(memCfg);
		MemFile_Reset(memData);
		
		MemFile_LoadFile_String(memCfg, FileSys_File("config.cfg"));
		MemFile_LoadFile(memData, FileSys_FindFile(".zovl"));
		
		entry[i].init = Config_GetInt(memCfg, "init_func");
		entry[i].destroy = Config_GetInt(memCfg, "dest_func");
		
		entry[i].vramStart = Config_GetInt(memCfg, "vram_addr");
		entry[i].vramEnd = entry[i].vramStart + memData->dataSize + Overlay_GetBssSize(memData);
		entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, memData, true);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].init);
		SwapBE(entry[i].destroy);
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
	}
	
	ItemList_Free(&list);
}

static void Build_Kaleido(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	KaleidoEntry* entry = rom->table.kaleido;
	RomOffset* romOff = &rom->offset;
	
	Rom_ItemList(&list, "rom/system/kaleido/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (s32 i = 0; i < list.num; i++) {
		char* file;
		printf_progress("Kaleido", i + 1, list.num);
		
		FileSys_Path(list.item[i]);
		
		MemFile_Reset(memCfg);
		MemFile_Reset(memData);
		
		file = FileSys_FindFile(".zovl");
		MemFile_LoadFile_String(memCfg, FileSys_File("config.cfg"));
		MemFile_LoadFile(memData, file);
		Patch_File(memData, file);
		
		entry[i].vramStart = Config_GetInt(memCfg, "vram_addr");
		entry[i].vramEnd = entry[i].vramStart + memData->dataSize + Overlay_GetBssSize(memData);
		entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, memData, NOCACHE_COMPRESS);
		entry[i].vromEnd = Dma_GetVRomEnd();
		
		SwapBE(entry[i].vromStart);
		SwapBE(entry[i].vromEnd);
		SwapBE(entry[i].vramStart);
		SwapBE(entry[i].vramEnd);
		
		if (i == 1) {  // PLAYER
			Mips64_SplitLoad(
				SegmentedToVirtual(SEG_CODE, romOff->table.player.init.hi - RELOC_CODE),
				SegmentedToVirtual(SEG_CODE, romOff->table.player.init.lo - RELOC_CODE),
				MIPS_REG_A0,
				Config_GetInt(memCfg, "init")
			);
			Mips64_SplitLoad(
				SegmentedToVirtual(SEG_CODE, romOff->table.player.dest.hi - RELOC_CODE),
				SegmentedToVirtual(SEG_CODE, romOff->table.player.dest.lo - RELOC_CODE),
				MIPS_REG_A0,
				Config_GetInt(memCfg, "dest")
			);
			Mips64_SplitLoad(
				SegmentedToVirtual(SEG_CODE, romOff->table.player.updt.hi - RELOC_CODE),
				SegmentedToVirtual(SEG_CODE, romOff->table.player.updt.lo - RELOC_CODE),
				MIPS_REG_A0,
				Config_GetInt(memCfg, "updt")
			);
			Mips64_SplitLoad(
				SegmentedToVirtual(SEG_CODE, romOff->table.player.draw.hi - RELOC_CODE),
				SegmentedToVirtual(SEG_CODE, romOff->table.player.draw.lo - RELOC_CODE),
				MIPS_REG_A0,
				Config_GetInt(memCfg, "draw")
			);
		} else {       // PAUSE_MENU
			Mips64_SplitLoad(
				SegmentedToVirtual(SEG_CODE, romOff->table.pauseMenu.updt.hi - RELOC_CODE),
				SegmentedToVirtual(SEG_CODE, romOff->table.pauseMenu.updt.lo - RELOC_CODE),
				MIPS_REG_A0,
				Config_GetInt(memCfg, "updt")
			);
			Mips64_SplitLoad(
				SegmentedToVirtual(SEG_CODE, romOff->table.pauseMenu.draw.hi - RELOC_CODE),
				SegmentedToVirtual(SEG_CODE, romOff->table.pauseMenu.draw.lo - RELOC_CODE),
				MIPS_REG_A0,
				Config_GetInt(memCfg, "draw")
			);
		}
	}
	
	ItemList_Free(&list);
}

static void Build_Skybox(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	
	Rom_ItemList(&list, "rom/system/skybox/", SORT_NUMERICAL, LIST_FOLDERS);
	
	for (s32 i = 0; i < list.num; i++) {
		if (list.item[i] == NULL)
			continue;
		
		printf_progress("Skybox", i + 1, list.num);
		
		u32 texId = 941 + i * 2;
		u32 palId = 942 + i * 2;
		
		FileSys_Path(list.item[i]);
		
		MemFile_Reset(memData);
		MemFile_LoadFile(memData, FileSys_FindFile(".tex"));
		Dma_WriteEntry(rom, texId, memData, true);
		
		MemFile_Reset(memData);
		MemFile_LoadFile(memData, FileSys_FindFile(".pal"));
		Dma_WriteEntry(rom, palId, memData, true);
	}
	
	ItemList_Free(&list);
}

static void Build_Static(Rom* rom, MemFile* memData, MemFile* memCfg) {
	ItemList list = ItemList_Initialize();
	
	Rom_ItemList(&list, "rom/system/static/", SORT_NO, LIST_FILES);
	
	foreach(i, gSystem_OoT) {
		s32 id = -1;
		s32 k = 0;
		u32 start, end;
		u32* data;
		s32 compress;
		
		forlist(j, list) {
			if (StrEndCase(list.item[j], xFmt("%s.bin", gSystem_OoT[i].name))) {
				id = gSystem_OoT[i].id;
				k = j;
			}
		}
		
		printf_progress("Static", i + 1, ArrayCount(gSystem_OoT));
		
		if (id < 0) {
			printf_warning("Missing [%s]", gSystem_OoT[i].name);
			continue;
		}
		MemFile mem;
		char* table;
		
		switch (id) {
			case DMA_ID_BOOT:
				Patch_File(&rom->boot, list.item[k]);
				MemFile_Seek(&rom->file, RELOC_BOOT);
				MemFile_Append(&rom->file, &rom->boot);
				
				continue;
				break;
				
			case DMA_ID_CODE:
			
				Patch_File(&rom->code, list.item[k]);
				
				Dma_WriteEntry(rom, id, &rom->code, NOCACHE_COMPRESS);
				
				continue;
				break;
				
			case DMA_ID_LINK_ANIMATION:
				Dma_WriteEntry(rom, id, &rom->playerAnim, false);
				continue;
				break;
				
			case DMA_ID_MESSAGE_DATA_STATIC_NES:
			case DMA_ID_MESSAGE_DATA_STATIC_STAFF:
				mem = MemFile_Initialize();
				table = xStrDup(list.item[k]);
				StrRep(table, ".bin", ".tbl");
				MemFile_LoadFile(&mem, table);
				
				if (StrStr(Basename(table), "NES"))
					memcpy(rom->table.nesMsg, mem.data, mem.dataSize);
				
				else
					memcpy(rom->table.staffMsg, mem.data, mem.dataSize);
				
				MemFile_Free(&mem);
				
				break;
		}
		
		MemFile_Reset(memData);
		MemFile_LoadFile(memData, list.item[k]);
		
		switch (id) {
			case DMA_ID_Z_SELECT_STATIC:
			case DMA_ID_NINTENDO_ROGO_STATIC:
			case DMA_ID_TITLE_STATIC:
			case DMA_ID_ELF_MESSAGE_FIELD:
			case DMA_ID_ELF_MESSAGE_YDAN:
				compress = COMPRESS;
				break;
			default:
				compress = false;
				break;
		}
		
		if (Patch_File(memData, list.item[k]) && compress)
			compress = NOCACHE_COMPRESS;
		
		start = Dma_WriteEntry(rom, id, memData, compress);
		end = Dma_GetVRomEnd();
		
		switch (id) {
			case DMA_ID_ELF_MESSAGE_FIELD:
				data = SegmentedToVirtual(SEG_CODE, 0xB9E6A8 - RELOC_CODE);
				data[0] = ReadBE(start);
				data[1] = ReadBE(end);
				
				break;
			case DMA_ID_ELF_MESSAGE_YDAN:
				data = SegmentedToVirtual(SEG_CODE, 0xB9E6A8 - RELOC_CODE);
				data[2] = ReadBE(start);
				data[3] = ReadBE(end);
				
				break;
		}
	}
	
	ItemList_Free(&list);
}

static void Build_EntranceTable(Rom* rom, MemFile* memData, MemFile* memCfg) {
	EntranceInfo* ent = rom->table.entrance;
	
	MemFile_Reset(memData);
	MemFile_LoadFile_String(memData, "rom/system/entrance_table.cfg");
	
	for (s32 i = 0; i < rom->table.num.entrance; i++, ent++) {
		ItemList list = ItemList_Initialize();
		
		Config_GetArray(memData, xFmt("0x%04X", i), &list);
		
		if (list.num != 6)
			printf_error("Entry 0x%04X does not have 6 items", i);
		
		ent->scene = Value_Hex(list.item[0]);
		ent->spawn = Value_Hex(list.item[1]);
		ent->continueBgm = Value_Bool(list.item[2]);
		ent->titleCard = Value_Bool(list.item[3]);
		ent->fadeIn = Transition_GetType(list.item[4]);
		ent->fadeOut = Transition_GetType(list.item[5]);
		
		ItemList_Free(&list);
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # Global                              #
// # # # # # # # # # # # # # # # # # # # #

void Rom_New(Rom* rom, char* romName) {
	u16* addr;
	
	rom->file = MemFile_Initialize();
	MemFile_Alloc(&rom->file, MbToBin(128));
	if (MemFile_LoadFile(&rom->file, romName))
		printf_error_align("Error Opening", "%s", romName);
	sBaseromSize = rom->file.dataSize;
	SetSegment(0x0, rom->file.data);
	
	if (strcmp(SegmentedToVirtual(0, 0xBCF8F0), "NOT MARIO CLUB VERSION")) {
		printf_warning("Provided rom, [%s], isn't " PRNT_YELW "OOT MQ DEBUG PAL", romName);
		printf_error("z64rom does not support other roms currently.");
	}
	
	rom->offset.table.dmaTable = 0x012F70;
	rom->offset.table.objTable = 0xB9E6C8;
	rom->offset.table.actorTable = 0xB8D440;
	rom->offset.table.effectTable = 0xB8CB50;
	rom->offset.table.stateTable = 0xB969D0;
	rom->offset.table.sceneTable = 0xBA0BB0;
	rom->offset.table.kaleidoTable = 0xBA4340;
	
	rom->offset.table.seqFontTbl = 0xBCC4E0;
	rom->offset.table.seqTable = 0xBCC6A0;
	rom->offset.table.fontTable = 0xBCC270;
	rom->offset.table.sampleTable = 0xBCCD90;
	
	rom->offset.table.nesEntryTable = 0xBC24C0;
	rom->offset.table.staffEntryTable = 0xBCA908;
	
	rom->offset.table.entranceTable = 0xB9F360;
	rom->offset.table.restrictionFlags = 0x00B9CA10;
	
	addr = SegmentedToVirtual(0x0, 0xB5A4AE);
	rom->offset.segment.seqRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
	rom->offset.segment.seqRom |= ReadBE(addr[2]);
	
	addr = SegmentedToVirtual(0x0, 0xB5A4C2);
	rom->offset.segment.fontRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
	rom->offset.segment.fontRom |= ReadBE(addr[2]);
	
	addr = SegmentedToVirtual(0x0, 0xB5A4D6);
	rom->offset.segment.smplRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
	rom->offset.segment.smplRom |= ReadBE(addr[2]);
	
	rom->table.num.dma = 1548;
	rom->table.num.obj = 402;
	rom->table.num.actor = 471;
	rom->table.num.effect = 37;
	rom->table.num.state = 6;
	rom->table.num.scene = 110;
	rom->table.num.kaleido = 2;
	rom->table.num.entrance = 1556;
	
	rom->offset.table.player.init = (HiLo) {
		0x00B288F8, 0x00B28900
	};
	rom->offset.table.player.dest = (HiLo) {
		0x00B28908, 0x00B28914
	};
	rom->offset.table.player.updt = (HiLo) {
		0x00B2891C, 0x00B28928
	};
	rom->offset.table.player.draw = (HiLo) {
		0x00B28930, 0x00B2893C
	};
	
	rom->offset.table.pauseMenu.updt = (HiLo) {
		0x00B33208, 0x00B3320C
	};
	rom->offset.table.pauseMenu.draw = (HiLo) {
		0x00B33218, 0x00B33220
	};
	
	rom->table.dma = SegmentedToVirtual(0x0, rom->offset.table.dmaTable);
	rom->table.object = SegmentedToVirtual(0x0, rom->offset.table.objTable);
	rom->table.actor = SegmentedToVirtual(0x0, rom->offset.table.actorTable);
	rom->table.effect = SegmentedToVirtual(0x0, rom->offset.table.effectTable);
	rom->table.state = SegmentedToVirtual(0x0, rom->offset.table.stateTable);
	rom->table.scene = SegmentedToVirtual(0x0, rom->offset.table.sceneTable);
	rom->table.kaleido = SegmentedToVirtual(0x0, rom->offset.table.kaleidoTable);
	
	rom->table.nesMsg = SegmentedToVirtual(0x0, rom->offset.table.nesEntryTable);
	rom->table.staffMsg = SegmentedToVirtual(0x0, rom->offset.table.staffEntryTable);
	
	rom->table.entrance = SegmentedToVirtual(0x0, rom->offset.table.entranceTable);
	rom->table.restrictionFlags = SegmentedToVirtual(0x0, rom->offset.table.restrictionFlags);
	
	rom->mem.sampleTbl = MemFile_Initialize();
	rom->mem.fontTbl = MemFile_Initialize();
	rom->mem.seqTbl = MemFile_Initialize();
	rom->mem.seqFontTbl = MemFile_Initialize();
	MemFile_Alloc(&rom->mem.sampleTbl, MbToBin(0.1));
	MemFile_Alloc(&rom->mem.fontTbl, MbToBin(0.1));
	MemFile_Alloc(&rom->mem.seqTbl, MbToBin(0.1));
	MemFile_Alloc(&rom->mem.seqFontTbl, MbToBin(0.1));
	
	if (!gDumpFlag) {
		s32 targetId[] = {
			DMA_ID_CODE,
			DMA_ID_BOOT
		};
		MemFile* targetMem[] = {
			&rom->code,
			&rom->boot
		};
		char* targetFile[] = {
			"z_code.bin",
			"z_boot.bin"
		};
		u8 targetSegment[] = {
			SEG_CODE,
			SEG_BOOT
		};
		
		foreach(k, targetId) {
			s32 id = -1;
			char* file;
			
			foreach(i, gSystem_OoT) {
				if (gSystem_OoT[i].id == targetId[k])
					id = i;
			}
			
			if (id == -1)
				printf_error("Could not solve ID for [%s]...", targetFile[k]);
			
			file = xFmt("rom/system/static/%s.bin", gSystem_OoT[id].name);
			if (!Sys_Stat(file)) file = xFmt("rom/system/static/%s/%s.bin", gVanilla, gSystem_OoT[id].name);
			if (!Sys_Stat(file)) printf_error("Could not find [%s.bin]", gSystem_OoT[id].name);
			
			MemFile_LoadFile(targetMem[k], file);
			SetSegment(targetSegment[k], targetMem[k]->data);
		}
		
		rom->table.state = SegmentedToVirtual(SEG_CODE, rom->offset.table.stateTable - RELOC_CODE);
		rom->table.kaleido = SegmentedToVirtual(SEG_CODE, rom->offset.table.kaleidoTable - RELOC_CODE);
		rom->table.nesMsg = SegmentedToVirtual(SEG_CODE, rom->offset.table.nesEntryTable - RELOC_CODE);
		rom->table.staffMsg = SegmentedToVirtual(SEG_CODE, rom->offset.table.staffEntryTable - RELOC_CODE);
		rom->table.entrance = SegmentedToVirtual(SEG_CODE, rom->offset.table.entranceTable - RELOC_CODE);
		rom->table.restrictionFlags = SegmentedToVirtual(SEG_CODE, rom->offset.table.restrictionFlags - RELOC_CODE);
		
		MemFile_Alloc(&rom->playerAnim, MbToBin(16));
	}
}

void Rom_Free(Rom* rom) {
	MemFile_Free(&rom->file);
	MemFile_Free(&rom->config);
	MemFile_Free(&rom->mem.sampleTbl);
	MemFile_Free(&rom->mem.fontTbl);
	MemFile_Free(&rom->mem.seqTbl);
	MemFile_Free(&rom->mem.seqFontTbl);
	MemFile_Free(&rom->code);
	MemFile_Free(&rom->boot);
	MemFile_Free(&rom->playerAnim);
	memset(rom, 0, sizeof(struct Rom));
}

void Rom_Dump(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	
	MemFile_Alloc(&dataFile, 0x460000); // Slightly larger than audiotable
	MemFile_Alloc(&config, 0x25000);
	
	printf_info_align("Dumping Rom", PRNT_REDD "%s", Filename(rom->file.info.name));
	
	FileSys_MakePath(true);
	Dump_Actor(rom, &dataFile, &config);
	Dump_Effect(rom, &dataFile, &config);
	Dump_Object(rom, &dataFile, &config);
	Dump_Scene(rom, &dataFile, &config);
	Dump_State(rom, &dataFile, &config);
	Dump_Kaleido(rom, &dataFile, &config);
	Dump_Static(rom, &dataFile, &config);
	Dump_Skybox(rom, &dataFile, &config);
	Dump_EntranceTable(rom, &dataFile, &config);
	Audio_DumpSoundFont(rom, &dataFile, &config);
	Audio_DumpSequence(rom, &dataFile, &config);
	Audio_DumpSampleTable(rom, &dataFile, &config);
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

void Rom_Build(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	
	#define Dma_FreeEntry(...) { \
			if (NARGS(__VA_ARGS__) == 3) \
			Dma_FreeEntry(VA1(__VA_ARGS__), VA2(__VA_ARGS__), VA3(__VA_ARGS__)); \
			else \
			{ Dma_FreeEntry(VA1(__VA_ARGS__), VA2(__VA_ARGS__), VA3(__VA_ARGS__)); Dma_WriteFlag(VA2(__VA_ARGS__), false); } } \

	
	Patch_Init();
	
	Text_Build(rom);
	
	MemFile_Alloc(&dataFile, 0x460000);
	MemFile_Alloc(&config, 0x25000);
	
	MemFile_Params(&dataFile, MEM_REALLOC, true, MEM_END);
	MemFile_Params(&config, MEM_REALLOC, true, MEM_END);
	
	printf_info_align("Load Baserom", PRNT_REDD "%s", Filename(rom->file.info.name));
	printf_info_align("Build Rom", PRNT_BLUE "%s",  gBuildrom[gBuildTarget]);
	
	ExtensionTable_Init(rom);
	
	Dma_FreeEntry(rom, DMA_ID_UNUSED_3, 0x10, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_UNUSED_4, 0x10, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_UNUSED_5, 0x10, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_LINK_ANIMATION, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_24_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_FIELD_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_DUNGEON_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_GAMEOVER_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_NES_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_GER_STATIC, 0x1000);
	Dma_FreeEntry(rom, DMA_ID_ICON_ITEM_FRA_STATIC, 0x1000);
	Dma_FreeEntry(rom, DMA_ID_ITEM_NAME_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MAP_NAME_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_DO_ACTION_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MESSAGE_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MESSAGE_TEXTURE_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_NES_FONT_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_NES, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_GER, 0x1000);
	Dma_FreeEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_FRA, 0x1000);
	Dma_FreeEntry(rom, DMA_ID_MESSAGE_DATA_STATIC_STAFF, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MAP_GRAND_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MAP_I_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_MAP_48X85_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_CODE, 0x10, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_Z_SELECT_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_NINTENDO_ROGO_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_TITLE_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_PARAMETER_STATIC, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ELF_MESSAGE_FIELD, 0x1000, ReserveSlot);
	Dma_FreeEntry(rom, DMA_ID_ELF_MESSAGE_YDAN, 0x1000, ReserveSlot);
	Dma_FreeGroup(rom, DMA_ACTOR);
	Dma_FreeGroup(rom, DMA_STATE);
	Dma_FreeGroup(rom, DMA_KALEIDO);
	Dma_FreeGroup(rom, DMA_EFFECT);
	Dma_FreeGroup(rom, DMA_OBJECT);
	Dma_FreeGroup(rom, DMA_SCENES);
	Dma_FreeGroup(rom, DMA_PLACE_NAME);
	Dma_FreeGroup(rom, DMA_SKYBOX_TEXEL);
	Dma_FreeGroup(rom, DMA_UNUSED);
	
#undef Dma_FreeEntry
	Dma_CombineSlots();
	
	if (gPrintInfo) {
		Dma_PrintfSlots(rom, "Marked Free", gSlotHead);
	}
	
	ExtensionTable_Alloc(rom);
	
	Log("Build");
	Audio_BuildSampleTable(rom, &dataFile, &config);
	Audio_BuildSoundFont(rom, &dataFile, &config);
	Audio_BuildSequence(rom, &dataFile, &config);
	Audio_UpdateSegments(rom);
	
	Build_Actor(rom, &dataFile, &config);
	Build_Effect(rom, &dataFile, &config);
	Build_Object(rom, &dataFile, &config);
	Build_Scene(rom, &dataFile, &config);
	
	Build_EntranceTable(rom, &dataFile, &config);
	
	Build_State(rom, &dataFile, &config);
	Build_Kaleido(rom, &dataFile, &config);
	Build_Skybox(rom, &dataFile, &config);
	
	printf_info("Patching");
	Patch_File(&rom->file, NULL);
	
	Build_Static(rom, &dataFile, &config);
	
	if (Sys_Stat("rom/lib_user/z_lib_user.bin")) {
		MemFile_Reset(&dataFile);
		MemFile_LoadFile(&dataFile, "rom/lib_user/z_lib_user.bin");
		if (dataFile.dataSize > 0xB5000)
			printf_error("z_lib_user.bin is bigger than %.2f MB. This wont fit into the RAM!", BinToMb(0xB5000));
		Dma_WriteEntry(rom, DMA_ID_UNUSED_3, &dataFile, true);
	}
	
	if (gPrintInfo) {
		if (!gCompressFlag)
			Dma_PrintfSlots(rom, "Left Free", gSlotHead);
		
		else
			Dma_PrintfSlots(rom, "Left Free", gSlotYazHead);
	}
	
	Dma_UpdateRomSize(rom);
	fix_crc(rom->file.data);
	MemFile_SaveFile(&rom->file, gBuildrom[gBuildTarget]);
	
	if (gCompressFlag)
		printf_info_align("Compression", "[%.1f%c]", ((f32)rom->file.dataSize / sBaseromSize) * 100.0, '%');
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
	Patch_Free();
}

void Rom_ItemList(ItemList* list, const char* path, bool isNum, ListFlag flags) {
	ItemList vanilla = ItemList_Initialize();
	ItemList modified = ItemList_Initialize();
	ItemList result = ItemList_Initialize();
	
	ItemList_List(&vanilla, xFmt("%s%s/", path, gVanilla), 0, flags | LIST_RELATIVE);
	
	ItemList_SetFilter(&modified, FILTER_WORD, gVanilla);
	ItemList_List(&modified, path, 0, flags | LIST_NO_DOT | LIST_RELATIVE);
	
	if (isNum) {
		ItemList_NumericalSort(&vanilla);
		ItemList_NumericalSort(&modified);
	}
	
	ItemList_Validate(list);
	list->item = xAlloc(sizeof(u8*) * (modified.num + vanilla.num));
	
	if (isNum) {
		u32 maxNum = 0;
		
		for (s32 i = 0; i < modified.num; i++) {
			if (modified.item[i] == NULL)
				continue;
			if (Value_Int(modified.item[i]) > maxNum) {
				maxNum = Value_Int(modified.item[i]);
			}
		}
		
		for (s32 i = 0; i < vanilla.num; i++) {
			if (vanilla.item[i] == NULL)
				continue;
			if (Value_Int(vanilla.item[i]) > maxNum)
				maxNum = Value_Int(vanilla.item[i]);
		}
		
		for (s32 i = 0; i < maxNum + 1; i++) {
			if (i < modified.num && modified.item[i] && Value_Int(modified.item[i]) == i) {
				list->item[list->num] = xFmt("%s%s", path, modified.item[i]);
			} else if (i < vanilla.num && vanilla.item[i] && Value_Int(vanilla.item[i]) == i) {
				list->item[list->num] = xFmt("%s%s/%s", path, gVanilla, vanilla.item[i]);
			} else {
				list->item[list->num] = NULL;
			}
			list->num++;
		}
	} else {
		u32 i = 0;
		
		while (i < modified.num) {
			list->item[list->num] = xFmt("%s%s", path, modified.item[i]);
			list->num++;
			i++;
		}
		
		i = 0;
		while (i < vanilla.num) {
			u32 cont = 0;
			for (s32 j = 0; j < list->num; j++) {
				if (!strcmp(vanilla.item[i], Filename(list->item[j]))) {
					cont = 1;
					i++;
					break;
				}
			}
			
			if (cont) continue;
			
			list->item[list->num] = xFmt("%s%s/%s", path, gVanilla, vanilla.item[i]);
			list->num++;
			i++;
		}
	}
	
	u32 size = 0;
	
	for (s32 i = 0; i < list->num; i++) {
		if (!list->item[i])
			continue;
		size += strlen(list->item[i]) + 1;
	}
	
	result.num = list->num;
	Calloc(result.buffer, size);
	Calloc(result.item, sizeof(u8*) * list->num);
	
	for (s32 i = 0; i < list->num; i++) {
		if (!list->item[i])
			continue;
		
		result.item[i] = &result.buffer[result.writePoint];
		strcpy(result.item[i], list->item[i]);
		result.writePoint += strlen(result.item[i]) + 1;
	}
	
	*list = result;
	ItemList_Free(&vanilla);
	ItemList_Free(&modified);
}

void Rom_Compress(void) {
	#define THREAD_NUM 64
	Thread thread[THREAD_NUM];
	const char* path[] = {
		"rom/actor/",
		"rom/effect/",
		"rom/object/",
		"rom/scene/",
		
		"rom/system/skybox/",
		"rom/system/skybox/",
		"rom/system/state/",
	};
	const char* ext[] = {
		".zovl",
		".zovl",
		".zobj",
		".zscene",
		
		".pal",
		".tex",
		".zovl",
	};
	
	if (gThreading)
		ThreadLock_Init();
	
	foreach(o, path) {
		ItemList list = ItemList_Initialize();
		s32 i = 0;
		
		Rom_ItemList(&list, path[o], SORT_NUMERICAL, LIST_FOLDERS);
		
		printf_info_align("Compressing", "[%d / %d]", o + 1, ArrayCount(path));
		
		while (i < list.num) {
			s32 skipList[THREAD_NUM] = { 0 };
			u32 target = Clamp(list.num - i, 0, THREAD_NUM);
			
			printf_progress("Files", i + 1, list.num);
			for (s32 j = 0; j < target; j++) {
				char* file;
				
				if (list.item[i + j]) {
					FileSys_Path(list.item[i + j]);
					if ((file = FileSys_FindFile(ext[o]))) {
						ThreadLock_Create(&thread[j], Yaz_EncodeThread, file);
						continue;
					}
				}
				
				skipList[j] = 1;
			}
			
			for (s32 j = 0; j < target; j++)
				if (skipList[j] != 1)
					ThreadLock_Join(&thread[j]);
			
			i += THREAD_NUM;
		}
		Terminal_ClearLines(3);
		
		ItemList_Free(&list);
	}
	
	if (gThreading)
		ThreadLock_Free();
	
#undef THREAD_NUM
}

void Rom_DeleteUnusedContent(void) {
	ItemList list = ItemList_Initialize();
	char* item;
	u32 id;
	
	ItemList_List(&list, xFmt("rom/actor/%s/", gVanilla), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(&list);
	for (s32 i = 0; i < ArrayCount(gBetaFlag_Actor_OoT); i++) {
		id = gBetaFlag_Actor_OoT[i];
		
		if (list.item[id] == NULL || id >= list.num)
			continue;
		
		item = xFmt("rom/actor/%s/%s", gVanilla, list.item[id]);
		
		printf_info("Delete [%s]", item);
		Sys_Delete_Recursive(item);
	}
	ItemList_Free(&list);
	
	ItemList_List(&list, xFmt("rom/object/%s/", gVanilla), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(&list);
	for (s32 i = 0; i < ArrayCount(gBetaFlag_Object_OoT); i++) {
		id = gBetaFlag_Object_OoT[i];
		
		if (list.item[id] == NULL || id >= list.num)
			continue;
		
		item = xFmt("rom/object/%s/%s", gVanilla, list.item[id]);
		
		printf_info("Delete [%s]", item);
		Sys_Delete_Recursive(item);
	}
	ItemList_Free(&list);
	
	ItemList_List(&list, xFmt("rom/scene/%s/", gVanilla), 0, LIST_FOLDERS | LIST_RELATIVE);
	ItemList_NumericalSort(&list);
	for (s32 i = 0; i < ArrayCount(gBetaFlag_Scene_OoT); i++) {
		id = gBetaFlag_Scene_OoT[i];
		
		if (list.item[id] == NULL || id >= list.num)
			continue;
		
		item = xFmt("rom/scene/%s/%s", gVanilla, list.item[id]);
		
		printf_info("Delete [%s]", item);
		Sys_Delete_Recursive(item);
	}
	ItemList_Free(&list);
}

s32 Rom_Extract(MemFile* mem, RomFile rom, char* name) {
	if (rom.size == 0)
		return 0;
	MemFile_Reset(mem);
	mem->dataSize = rom.size;
	MemFile_Realloc(mem, rom.size);
	MemFile_Write(mem, rom.data, rom.size);
	MemFile_SaveFile(mem, name);
	
	return 1;
}

void AudioOnly_Dump(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	
	MemFile_Alloc(&dataFile, 0x460000); // Slightly larger than audiotable
	MemFile_Alloc(&config, 0x25000);
	
	printf_info_align("Dumping Rom", PRNT_REDD "%s", Filename(rom->file.info.name));
	
	Audio_DumpSoundFont(rom, &dataFile, &config);
	Audio_DumpSequence(rom, &dataFile, &config);
	Audio_DumpSampleTable(rom, &dataFile, &config);
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

void AudioOnly_Build(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	
	MemFile_Alloc(&dataFile, 0x460000);
	MemFile_Alloc(&config, 0x25000);
	
	MemFile_Params(&dataFile, MEM_REALLOC, true, MEM_END);
	MemFile_Params(&config, MEM_REALLOC, true, MEM_END);
	
	Audio_BuildSampleTable(rom, &dataFile, &config);
	Audio_BuildSoundFont(rom, &dataFile, &config);
	Audio_BuildSequence(rom, &dataFile, &config);
	
	MemFile_SaveFile(&rom->mem.sampleTbl, "table_sample.bin");
	MemFile_SaveFile(&rom->mem.fontTbl, "table_font.bin");
	MemFile_SaveFile(&rom->mem.seqTbl, "table_seq.bin");
	MemFile_SaveFile(&rom->mem.seqFontTbl, "table_seqfont.bin");
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

void Rom_Debug_ActorEntry(Rom* rom, u32 id) {
	ActorEntry* actorTable = rom->table.actor;
	s32 i = 0;
	
	printf_info("" PRNT_REDD "0x%04X-%s " PRNT_RSET "[%d]", id, gActorName_OoT[id], id);
	printf_info("Actor\t[%08d] [%08X]", id, VirtualToSegmented(0x0, &actorTable[id]));
	printf_info("vRAM\t" PRNT_PRPL "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(actorTable[id].vramStart), ReadBE(actorTable[id].vramEnd), ReadBE(actorTable[id].vramEnd) - ReadBE(actorTable[id].vramStart));
	printf_info("vROM\t" PRNT_YELW "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(actorTable[id].vromStart), ReadBE(actorTable[id].vromEnd), ReadBE(actorTable[id].vromEnd) - ReadBE(actorTable[id].vromStart));
	printf_info("InitVars\t"PRNT_GREN "[%08X]"PRNT_RSET, ReadBE(actorTable[id].initInfo));
	printf_info("BssSize\t[%08X]", (ReadBE(actorTable[id].vramEnd) - ReadBE(actorTable[id].vramStart)) - (ReadBE(actorTable[id].vromEnd) - ReadBE(actorTable[id].vromStart)) );
	
	if ((ReadBE(actorTable[id].vramEnd) - ReadBE(actorTable[id].vramStart)) == 0)
		return;
	printf("\n");
	for (;; i++) {
		if (rom->table.dma[i].vromStart == actorTable[id].vromStart &&
			rom->table.dma[i].vromEnd == actorTable[id].vromEnd)
			break;
		if (i > rom->table.num.dma) {
			printf_warning("Could not find DMA enrty");
			
			return;
		}
	}
	
	printf_info("Dma Entry\t[%08d] [%08X]", i, VirtualToSegmented(0x0, &rom->table.dma[i]));
	printf_info("vROM\t" PRNT_YELW "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(rom->table.dma[i].vromStart), ReadBE(rom->table.dma[i].vromEnd), ReadBE(rom->table.dma[i].vromEnd) - ReadBE(rom->table.dma[i].vromStart));
}

void Rom_Debug_DmaEntry(Rom* rom, u32 id) {
	printf_info("Dma Entry\t[%08d] [%08X]", id, VirtualToSegmented(0x0, &rom->table.dma[id]));
	printf_info("vROM\t" PRNT_YELW "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(rom->table.dma[id].vromStart), ReadBE(rom->table.dma[id].vromEnd), ReadBE(rom->table.dma[id].vromEnd) - ReadBE(rom->table.dma[id].vromStart));
	printf_info("pROM\t" PRNT_YELW "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(rom->table.dma[id].romStart), ReadBE(rom->table.dma[id].romEnd), ClampMin((s32)(ReadBE(rom->table.dma[id].romEnd) - ReadBE(rom->table.dma[id].romStart)), 0));
}

void Rom_Debug_SceneEntry(Rom* rom, u32 id) {
	printf_info("Dma Entry\t[%08d] [%08X]", id, VirtualToSegmented(0x0, &rom->table.scene[id]));
	printf_info("scene ROM\t" PRNT_YELW "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(rom->table.scene[id].vromStart), ReadBE(rom->table.scene[id].vromEnd), ReadBE(rom->table.scene[id].vromEnd) - ReadBE(rom->table.scene[id].vromStart));
	printf_info("title ROM\t" PRNT_YELW "[%08X]-[%08X]"PRNT_RSET " Size 0x%X", ReadBE(rom->table.scene[id].titleVromStart), ReadBE(rom->table.scene[id].titleVromEnd), ClampMin((s32)(ReadBE(rom->table.scene[id].titleVromEnd) - ReadBE(rom->table.scene[id].titleVromStart)), 0));
}
