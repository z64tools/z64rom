#include "z64rom.h"

DirCtx gDir;

u32 TestSwap(u32 swapMe) {
	SwapBE(swapMe);
	
	return swapMe;
}

void Rom_ItemList(ItemList* list, bool isNum, bool isDir) {
	ItemList vanilla = { 0 };
	ItemList modified = { 0 };
	
	Dir_Enter(&gDir, ".vanilla/"); {
		Dir_ItemList(&gDir, &vanilla, isDir);
		
		Dir_Leave(&gDir);
	}
	Dir_ItemList(&gDir, &modified, isDir);
	
	if (isNum) {
		ItemList_NumericalSort(&vanilla);
		ItemList_NumericalSort(&modified);
	}
	
	*list = (ItemList) { 0 };
	list->item = Tmp_Alloc(sizeof(u8*) * (modified.num + vanilla.num));
	
	if (isNum) {
		u32 maxNum = 0;
		
		for (s32 i = 0; i < modified.num; i++) {
			if (modified.item[i] == NULL)
				continue;
			if (String_GetInt(modified.item[i]) > maxNum) {
				maxNum = String_GetInt(modified.item[i]);
			}
		}
		
		for (s32 i = 0; i < vanilla.num; i++) {
			if (vanilla.item[i] == NULL)
				continue;
			if (String_GetInt(vanilla.item[i]) > maxNum)
				maxNum = String_GetInt(vanilla.item[i]);
		}
		
		for (s32 i = 0; i < maxNum + 1; i++) {
			if (i < modified.num && modified.item[i] && String_GetInt(modified.item[i]) == i) {
				list->item[list->num] = Tmp_String(modified.item[i]);
			} else if (i < vanilla.num && vanilla.item[i] && String_GetInt(vanilla.item[i]) == i) {
				list->item[list->num] = Tmp_Printf(".vanilla/%s", vanilla.item[i]);
			} else {
				list->item[list->num] = NULL;
			}
			list->num++;
		}
	} else {
		u32 i = 0;
		
		while (i < modified.num) {
			list->item[list->num] = Tmp_String(modified.item[i]);
			list->num++;
			i++;
		}
		
		i = 0;
		while (i < vanilla.num) {
			u32 cont = 0;
			for (s32 j = 0; j < list->num; j++) {
				if (!strcmp(vanilla.item[i], list->item[j])) {
					cont = 1;
					i++;
					break;
				}
			}
			
			if (cont) continue;
			
			list->item[list->num] = Tmp_Printf(".vanilla/%s", vanilla.item[i]);
			list->num++;
			i++;
		}
	}
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

u32 Rom_Ovl_GetBssSize(MemFile* dataFile) {
	u32* bssSize;
	
	SetSegment(0x1, dataFile->data);
	bssSize = SegmentedToVirtual(0x1, dataFile->dataSize - 4);
	bssSize = SegmentedToVirtual(0x1, dataFile->dataSize - ReadBE(bssSize[0]));
	
	return ReadBE(bssSize[3]);
}

RestrictionFlag* Rom_GetRestrictionFlags(Rom* rom, u32 sceneIndex) {
	RestrictionFlag* flagList = rom->table.restrictionFlags;
	
	while (flagList->sceneIndex != 0xFF) {
		if (flagList->sceneIndex == sceneIndex)
			return flagList;
		
		flagList++;
	}
	
	return NULL;
}

void Rom_WriteRestrictionFlags(Rom* rom, MemFile* config, u32 sceneIndex) {
	char* flags = Config_Get(config, "restriction_flags");
	RestrictionFlag* rf = Rom_GetRestrictionFlags(rom, sceneIndex);
	
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

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

static void Rom_Config_Actor(MemFile* config, ActorEntry* actorOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Hex("vram_addr", ReadBE(actorOvl->vramStart));
	Config_WriteVar_Hex("init_vars", ReadBE(actorOvl->initInfo));
	Config_WriteVar_Int("alloc_type", ReadBE(actorOvl->allocType));
	MemFile_SaveFile_String(config, out);
}

static void Rom_Config_Effect(MemFile* config, EffectEntry* actorOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Hex("vram_addr", ReadBE(actorOvl->vramStart));
	Config_WriteVar_Hex("init_vars", ReadBE(actorOvl->initInfo));
	MemFile_SaveFile_String(config, out);
}

static void Rom_Config_GameState(MemFile* config, GameStateEntry* stateOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Hex("vram_addr", ReadBE(stateOvl->vramStart));
	Config_WriteVar_Hex("init_func", ReadBE(stateOvl->init));
	Config_WriteVar_Hex("dest_func", ReadBE(stateOvl->destroy));
	MemFile_SaveFile_String(config, out);
}

static void Rom_Config_Kaleido(Rom* rom, MemFile* config, u32 id, const char* name, char* out) {
	KaleidoEntry* entry = &rom->table.kaleido[id];
	u16* dataHi;
	u16* dataLo;
	u32 init;
	u32 dest;
	u32 updt;
	u32 draw;
	
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Hex("vram_addr", ReadBE(entry->vramStart));
	
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
		
		Config_WriteVar_Hex("init", init);
		Config_WriteVar_Hex("dest", dest);
		Config_WriteVar_Hex("updt", updt);
		Config_WriteVar_Hex("draw", draw);
	} else { // PAUSE_MENU
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.init.hi);
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.init.lo);
		init = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
		
		dataHi = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.updt.hi);
		dataLo = SegmentedToVirtual(0x0, rom->offset.table.pauseMenu.updt.lo);
		updt = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
		
		Config_WriteVar_Hex("init", init);
		Config_WriteVar_Hex("updt", updt);
	}
	
	MemFile_SaveFile_String(config, out);
}

static void Rom_Config_Scene(Rom* rom, MemFile* config, u32 id, const char* name, char* out) {
	#define FIWI \
		if (firstWritten > 0) { \
			MemFile_Printf(config, " | "); \
		} firstWritten++;
	
	SceneEntry* sceneEntry = &rom->table.scene[id];
	RestrictionFlag* rf = Rom_GetRestrictionFlags(rom, id);
	
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
#if 0
	Config_WriteVar_Int("unk_a", ReadBE(sceneEntry->unk_10));
	Config_WriteVar_Int("unk_b", ReadBE(sceneEntry->unk_12));
#endif
	Config_WriteVar_Int("scene_func_id", ReadBE(sceneEntry->config));
	
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

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

static void Rom_Patch_Config(Rom* rom, MemFile* dataFile, MemFile* config, char* file) {
	s32 lineNum;
	
	MemFile_Reset(config);
	MemFile_LoadFile_String(config, Dir_File(&gDir, file));
	lineNum = String_GetLineCount(config->data);
	
	for (s32 i = 0; i < lineNum; i++) {
		char* line = String_Line(config->data, i);
		char* word = String_Word(line, 2);
		
		if (line[0] != '0' && line[1] != 'x')
			continue;
		
		rom->file.seekPoint = String_GetHexInt(String_GetWord(line, 0));
		
		if (word[0] == '"') {
			word = Tmp_String(String_GetLine(String_Word(line, 2), 0));
			word = &word[1];
			
			for (s32 c = 0, j = 0; j < strlen(word); j++) {
				if (word[j] == '"')
					c = 1;
				if (c)
					word[j] = '\0';
			}
			
			MemFile_Printf(&rom->file, word);
		}
		if (word[0] == '0' && word[1] == 'x') {
			word = String_GetLine(word, 0);
			
			for (s32 o = 0, j = 2; j < strlen(word); j++) {
				u8* data = &rom->file.cast.u8[rom->file.seekPoint];
				u8 new;
				char strval[2] = {
					word[j],
					'\0'
				};
				
				if (word[j] == '#' || word[j] < ' ' || word[j] == '\0')
					break;
				
				if (word[j] == ' ' || (word[j] == '0' && word[j + 1] == 'x') || word[j] == 'x') {
					continue;
				}
				
				if (o % 2 == 0) {
					new = data[0] & 0x0F;
					new |= String_GetHexInt(strval) << 4;
					data[0] = new;
				} else {
					new = data[0] & 0xF0;
					new |= String_GetHexInt(strval) & 0xF;
					data[0] = new;
					rom->file.seekPoint++;
				}
				o++;
			}
		}
	}
}

static void Rom_Patch_Binary(Rom* rom, MemFile* dataFile, MemFile* config, char* file) {
	char* tmp = StrStr(String_GetBasename(file), "_0x");
	
	if (tmp == NULL)
		return;
	
	tmp = &tmp[1];
	rom->file.seekPoint = String_GetHexInt(tmp);
	MemFile_Reset(dataFile);
	MemFile_LoadFile(dataFile, Dir_File(&gDir, file));
	MemFile_Append(&rom->file, dataFile);
}

static void Rom_Build_Patch(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList list;
	
	Dir_ItemList_Recursive(&gDir, &list, ".cfg");
	
	for (s32 i = 0; i < list.num; i++) {
		Rom_Patch_Config(rom, dataFile, config, list.item[i]);
	}
	
	Dir_ItemList_Recursive(&gDir, &list, ".bin");
	
	for (s32 i = 0; i < list.num; i++) {
		Rom_Patch_Binary(rom, dataFile, config, list.item[i]);
	}
}

static void Rom_Build_Code(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList list;
	
	Dir_ItemList_Recursive(&gDir, &list, ".bin");
	
	for (s32 i = 0; i < list.num; i++) {
		char* fileCfg = Dir_File(&gDir, "%s%s.cfg", String_GetPath(list.item[i]), String_GetBasename(list.item[i]));
		
		MemFile_Reset(dataFile);
		MemFile_Reset(config);
		
		if (!Sys_Stat(fileCfg))
			printf_error("Could not find [%s]", fileCfg);
		
		if (MemFile_LoadFile(dataFile, Dir_File(&gDir, list.item[i]))) printf_error("Exiting...");
		if (MemFile_LoadFile_String(config, fileCfg)) printf_error("Exiting...");
		
		rom->file.seekPoint = Config_GetInt(config, "rom");
		MemFile_Append(&rom->file, dataFile);
	}
}

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

void Rom_Dump(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	RomFile rf;
	
	Dir_SetParam(&gDir, DIR__MAKE_ON_ENTER);
	MemFile_Malloc(&dataFile, 0x460000); // Slightly larger than audiotable
	MemFile_Malloc(&config, 0x25000);
	
	printf_info_align("Dump Rom", PRNT_PRPL "%s", rom->file.info.name);
	
	Dir_Enter(&gDir, "rom/"); {
		Dir_Enter(&gDir, "actor/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.actor; i++) {
				rf = Dma_RomFile_Actor(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Actor", i + 1, rom->table.num.actor);
				Dir_Enter(&gDir, "0x%04X-%s/", i, gActorName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File(&gDir, "actor.zovl")))
						Rom_Config_Actor(&config, &rom->table.actor[i], gActorName_OoT[i], Dir_File(&gDir, "config.cfg"));
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "effect/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.effect; i++) {
				rf = Dma_RomFile_Effect(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Effect", i + 1, rom->table.num.effect);
				Dir_Enter(&gDir, "0x%04X-%s/", i, gEffectName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File(&gDir, "effect.zovl")))
						Rom_Config_Effect(&config, &rom->table.effect[i], gEffectName_OoT[i], Dir_File(&gDir, "config.cfg"));
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "object/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.obj; i++) {
				rf = Dma_RomFile_Object(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Object", i + 1, rom->table.num.obj);
				Dir_Enter(&gDir, "0x%04X-%s/", i, gObjectName_OoT[i]); {
					Rom_Extract(&dataFile, rf, Dir_File(&gDir, "object.zobj"));
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "scene/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.scene; i++) {
				rf = Dma_RomFile_Scene(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Scene", i + 1, rom->table.num.scene);
				Dir_Enter(&gDir, "0x%02X-%s/", i, gSceneName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File(&gDir, "scene.zscene"))) {
						u32* seg;
						u32 roomNum;
						u32 roomListSeg;
						u32* vromSeg;
						
						Rom_Config_Scene(rom, &config, i, gSceneName_OoT[i], Dir_File(&gDir, "config.cfg"));
						SetSegment(0x2, rf.data);
						seg = dataFile.data;
						
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
								char* out = Dir_File(&gDir, "room_%d.zroom", j);
								
								vromSeg = SegmentedToVirtual(0x2, roomListSeg + 8 * j);
								Rom_Extract(
									&dataFile,
									Rom_GetRomFile(rom, vromSeg[0], vromSeg[1]),
									out
								);
							}
						}
					}
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "system/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.state; i++) {
				rf = Dma_RomFile_GameState(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("System", i + 1, rom->table.num.state);
				Dir_Enter(&gDir, "GameState_%s/", gStateName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File(&gDir, "state.zovl")))
						Rom_Config_GameState(&config, &rom->table.state[i], gStateName_OoT[i], Dir_File(&gDir, "config.cfg"));
					
					Dir_Leave(&gDir);
				}
			}
			
			for (s32 i = 0; i < rom->table.num.kaleido; i++) {
				Dir_Enter(&gDir, "Kaleido_%s/", gKaleidoName_OoT[i]); {
					rf.size = ReadBE(rom->table.kaleido[i].vromEnd) - ReadBE(rom->table.kaleido[i].vromStart);
					rf.data = SegmentedToVirtual(0x0, ReadBE(rom->table.kaleido[i].vromStart));
					
					Rom_Extract(&dataFile, rf, Dir_File(&gDir, "overlay.zovl"));
					Rom_Config_Kaleido(rom, &config, i, gKaleidoName_OoT[i], Dir_File(&gDir, "config.cfg"));
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "static/.vanilla/"); {
			for (s32 i = 6; i <= DMA_ID_PARAMETER_STATIC; i++) {
				s32 name = i;
				if (i > DMA_ID_CODE && i < DMA_ID_OVL_MAP_MARK_DATA)
					i = DMA_ID_OVL_MAP_MARK_DATA;
				if (i > DMA_ID_OVL_MAP_MARK_DATA && i < DMA_ID_Z_SELECT_STATIC)
					i = DMA_ID_Z_SELECT_STATIC;
				
				if (name >= DMA_ID_Z_SELECT_STATIC)
					name -= 901;
				
				rf.romStart = ReadBE(rom->table.dma[i].vromStart);
				rf.romEnd = ReadBE(rom->table.dma[i].vromEnd);
				rf.size = rf.romEnd - rf.romStart;
				rf.data = SegmentedToVirtual(0x0, rf.romStart);
				
				Rom_Extract(&dataFile, rf, Dir_File(&gDir, "%s.bin", gSystemName_OoT[name]));
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "skybox/.vanilla/"); {
			for (s32 i = 0; i < 32; i++) {
				printf_progress("Skybox", i + 1, 32);
				
				Dir_Enter(&gDir, "%02d-%s/", i, gSkyboxName_OoT[i]); {
					rf.romStart = ReadBE(rom->table.dma[941 + i * 2].vromStart);
					rf.romEnd = ReadBE(rom->table.dma[941 + i * 2].vromEnd);
					rf.size = rf.romEnd - rf.romStart;
					rf.data = SegmentedToVirtual(0x0, rf.romStart);
					Rom_Extract(&dataFile, rf, Dir_File(&gDir, "texel.bin"));
					
					rf.romStart = ReadBE(rom->table.dma[942 + i * 2].vromStart);
					rf.romEnd = ReadBE(rom->table.dma[942 + i * 2].vromEnd);
					rf.size = rf.romEnd - rf.romStart;
					rf.data = SegmentedToVirtual(0x0, rf.romStart);
					Rom_Extract(&dataFile, rf, Dir_File(&gDir, "palette.bin"));
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "sound/"); {
			Rom_Dump_SoundFont(rom, &dataFile, &config);
			Rom_Dump_Sequences(rom, &dataFile, &config);
			Rom_Dump_Samples(rom, &dataFile, &config);
			
			Dir_Leave(&gDir);
		}
		
		Dir_Leave(&gDir);
	}
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

void Rom_Build(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	
	MemFile_Malloc(&dataFile, 0x460000);
	MemFile_Malloc(&config, 0x25000);
	MemFile_Params(&config, MEM_FILENAME, true, MEM_END);
	
	printf_info_align("Load Baserom", PRNT_PRPL "%s", rom->file.info.name);
	printf_info_align("Build Rom", PRNT_PRPL "build.z64");
	
	Dma_FreeEntry(rom, DMA_ID_UNUSED_3, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_3, false);
	Dma_FreeEntry(rom, DMA_ID_UNUSED_4, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_4, false);
	Dma_FreeEntry(rom, DMA_ID_UNUSED_5, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_5, false);
	
	Dma_FreeEntry(rom, DMA_ID_LINK_ANIMATION, 0x1000); Dma_WriteFlag(DMA_ID_LINK_ANIMATION, false);
	Dma_FreeEntry(rom, DMA_ID_TITLE_STATIC, 0x1000); Dma_WriteFlag(DMA_ID_TITLE_STATIC, false);
	Dma_FreeEntry(rom, DMA_ID_PARAMETER_STATIC, 0x1000); Dma_WriteFlag(DMA_ID_PARAMETER_STATIC, false);
	
	Dma_FreeGroup(rom, DMA_ACTOR);
	Dma_FreeGroup(rom, DMA_EFFECT);
	Dma_FreeGroup(rom, DMA_OBJECT);
	Dma_FreeGroup(rom, DMA_SCENES);
	Dma_FreeGroup(rom, DMA_SKYBOX_TEXEL);
	Dma_FreeGroup(rom, DMA_UNUSED);
	Dma_CombineSlots();
	
	if (gPrintInfo)
		Dma_PrintfSlots(rom);
	
	Dir_Enter(&gDir, "rom/"); {
		Dir_Enter(&gDir, "sound/"); {
			Dir_Enter(&gDir, "sample/"); {
				Rom_Build_SampleTable(rom, &dataFile, &config);
				
				Dir_Leave(&gDir);
			}
			Dir_Enter(&gDir, "soundfont/"); {
				Rom_Build_SoundFont(rom, &dataFile, &config);
				
				Dir_Leave(&gDir);
			}
			Dir_Enter(&gDir, "sequence/"); {
				Rom_Build_Sequence(rom, &dataFile, &config);
				
				Dir_Leave(&gDir);
			}
			Rom_Build_SetAudioSegment(rom);
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "actor/"); {
			ItemList actorList;
			ActorEntry* entry = rom->table.actor;
			Rom_ItemList(&actorList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < actorList.num; i++) {
				if (actorList.item[i] == NULL) {
					if (entry[i].vromStart && entry[i].vromEnd)
						entry[i] = (ActorEntry) { 0 };
					continue;
				}
				
				if (i >= 471) {
					printf_warning("Illegal action! Can't have more than " PRNT_REDD "0x01D6" PRNT_RSET " actors!", i);
					break;
				} else
					printf_progress("Actor", i + 1, actorList.num);
				
				Dir_Enter(&gDir, actorList.item[i]); {
					MemFile_Reset(&dataFile);
					MemFile_Reset(&config);
					MemFile_LoadFile(&dataFile, Dir_File(&gDir, "*.zovl"));
					MemFile_LoadFile_String(&config, Dir_File(&gDir, "config.cfg"));
					
					entry[i].allocType = Config_GetInt(&config, "alloc_type");
					entry[i].initInfo = Config_GetInt(&config, "init_vars");
					
					entry[i].vramStart = Config_GetInt(&config, "vram_addr");
					entry[i].vramEnd = entry[i].vramStart + dataFile.dataSize + Rom_Ovl_GetBssSize(&dataFile);
					
					entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
					entry[i].vromEnd = Dma_GetVRomEnd();
					
					SwapBE(entry[i].allocType);
					SwapBE(entry[i].initInfo);
					SwapBE(entry[i].vramStart);
					SwapBE(entry[i].vramEnd);
					SwapBE(entry[i].vromStart);
					SwapBE(entry[i].vromEnd);
					
					entry[i].loadedRamAddr = 0;
					entry[i].name = 0;
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "effect/"); {
			ItemList effectList;
			EffectEntry* entry = rom->table.effect;
			Rom_ItemList(&effectList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < effectList.num; i++) {
				if (effectList.item[i] == NULL) {
					entry[i] = (EffectEntry) { 0 };
					continue;
				}
				
				if (i >= 471) {
					printf_warning("Illegal action! Can't have more than " PRNT_REDD "0x01D6" PRNT_RSET " actors!", i);
					break;
				} else
					printf_progress("Effect", i + 1, effectList.num);
				
				Dir_Enter(&gDir, effectList.item[i]); {
					MemFile_Reset(&dataFile);
					MemFile_Reset(&config);
					MemFile_LoadFile(&dataFile, Dir_File(&gDir, "*.zovl"));
					MemFile_LoadFile_String(&config, Dir_File(&gDir, "config.cfg"));
					
					entry[i].initInfo = Config_GetInt(&config, "init_vars");
					
					entry[i].vramStart = Config_GetInt(&config, "vram_addr");
					entry[i].vramEnd = entry[i].vramStart + dataFile.dataSize + Rom_Ovl_GetBssSize(&dataFile);
					
					entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
					entry[i].vromEnd = Dma_GetVRomEnd();
					
					SwapBE(entry[i].initInfo);
					SwapBE(entry[i].vramStart);
					SwapBE(entry[i].vramEnd);
					SwapBE(entry[i].vromStart);
					SwapBE(entry[i].vromEnd);
					entry[i].loadedRamAddr = 0;
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "object/"); {
			ItemList objectList;
			ObjectEntry* entry = rom->table.object;
			Rom_ItemList(&objectList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < objectList.num; i++) {
				if (objectList.item[i] == NULL) {
					entry[i].vromStart = 0;
					entry[i].vromEnd = 0;
					
					continue;
				}
				
				Dir_Enter(&gDir, objectList.item[i]); {
					printf_progress("Object", i + 1, objectList.num);
					
					MemFile_Reset(&dataFile);
					MemFile_LoadFile(&dataFile, Dir_File(&gDir, "*.zobj"));
					
					entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, true);
					entry[i].vromEnd = Dma_GetVRomEnd();
					SwapBE(entry[i].vromStart);
					SwapBE(entry[i].vromEnd);
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "scene/"); {
			MemFile memRoom = MemFile_Initialize();
			ItemList sceneList;
			SceneEntry* entry = rom->table.scene;
			
			MemFile_Malloc(&memRoom, MbToBin(2));
			Rom_ItemList(&sceneList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < sceneList.num; i++) {
				if (sceneList.item[i] == NULL) {
					printf_error("Empty scene %d", i);
				}
				
				Dir_Enter(&gDir, sceneList.item[i]); {
					u32* seg;
					u32* vromSeg;
					u32 roomNum;
					u32 roomListSeg;
					
					printf_progress("Scene", i + 1, sceneList.num);
					
					MemFile_Reset(&config);
					MemFile_Reset(&dataFile);
					if (MemFile_LoadFile_String(&config, Dir_File(&gDir, "config.cfg"))) printf_error("Exiting...");
					if (MemFile_LoadFile(&dataFile, Dir_File(&gDir, "scene.zscene"))) printf_error("Exiting...");
					SetSegment(0x1, dataFile.data);
					seg = SegmentedToVirtual(0x1, 0);
					
					Rom_WriteRestrictionFlags(rom, &config, i);
					
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
						MemFile_Reset(&memRoom);
						if (Sys_Stat(Dir_File(&gDir, "room_%d.zroom", j))) {
							if (MemFile_LoadFile(&memRoom, Dir_File(&gDir, "room_%d.zroom", j)))
								printf_error("Exiting...");
						} else if (MemFile_LoadFile(&memRoom, Dir_File(&gDir, "room_%d.zmap", j)))
							printf_error("Exiting...");
						
						vromSeg[id + 0] = Dma_WriteEntry(rom, DMA_FIND_FREE, &memRoom, false);
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
					
					entry[i].config = Config_GetInt(&config, "scene_func_id");
					entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
					entry[i].vromEnd = Dma_GetVRomEnd();
					SwapBE(entry[i].vromStart);
					SwapBE(entry[i].vromEnd);
					
					Dir_Leave(&gDir);
				}
			}
			
			MemFile_Free(&memRoom);
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "system/"); {
			ItemList gameSysList;
			
			Rom_ItemList(&gameSysList, SORT_NO, IS_DIR);
			
			for (s32 i = 0; i < gameSysList.num; i++) {
				s32 id = 0;
				
				printf_progress("System", i + 1, gameSysList.num);
				
				// Get id based on name
				for (;; id++) {
					if (id >= ArrayCount(gStateName_OoT)) {
						id = 0;
						break;
					}
					if (StrStr(gameSysList.item[i], gStateName_OoT[id])) {
						id++;
						break;
					}
				}
				
				if (id == 0) {
					for (;; id--) {
						if (Abs(id) >= ArrayCount(gStateName_OoT)) {
							id = 0;
							break;
						}
						if (StrStr(gameSysList.item[i], gKaleidoName_OoT[Abs(id)])) {
							id--;
							break;
						}
					}
				}
				
				// GameState
				if (id > 0) {
					id--;
					
					Dir_Enter(&gDir, gameSysList.item[i]); {
						GameStateEntry* entry = rom->table.state;
						MemFile_Reset(&config);
						MemFile_Reset(&dataFile);
						
						if (MemFile_LoadFile_String(&config, Dir_File(&gDir, "config.cfg")))
							printf_error("Exiting");
						if (MemFile_LoadFile(&dataFile, Dir_File(&gDir, "*.zovl")))
							printf_error("Exiting");
						
						entry[id].init = Config_GetInt(&config, "init_func");
						entry[id].destroy = Config_GetInt(&config, "dest_func");
						
						entry[id].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
						entry[id].vromEnd = Dma_GetVRomEnd();
						
						entry[id].vramStart = Config_GetInt(&config, "vram_addr");
						entry[id].vramEnd = entry[id].vramStart + dataFile.dataSize + Rom_Ovl_GetBssSize(&dataFile);
						
						SwapBE(entry[id].init);
						SwapBE(entry[id].destroy);
						SwapBE(entry[id].vromStart);
						SwapBE(entry[id].vromEnd);
						SwapBE(entry[id].vramStart);
						SwapBE(entry[id].vramEnd);
						
						Dir_Leave(&gDir);
					}
				} else if (id < 0) {
					id = Abs(id) - 1;
					
					Dir_Enter(&gDir, gameSysList.item[i]); {
						KaleidoEntry* entry = rom->table.kaleido;
						RomOffset* romOff = &rom->offset;
						MemFile_Reset(&config);
						MemFile_Reset(&dataFile);
						
						if (MemFile_LoadFile_String(&config, Dir_File(&gDir, "config.cfg")))
							printf_error("Exiting");
						if (MemFile_LoadFile(&dataFile, Dir_File(&gDir, "*.zovl")))
							printf_error("Exiting");
						
						entry[id].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
						entry[id].vromEnd = Dma_GetVRomEnd();
						
						entry[id].vramStart = Config_GetInt(&config, "vram_addr");
						entry[id].vramEnd = entry[id].vramStart + dataFile.dataSize + Rom_Ovl_GetBssSize(&dataFile);
						
						SwapBE(entry[id].vromStart);
						SwapBE(entry[id].vromEnd);
						SwapBE(entry[id].vramStart);
						SwapBE(entry[id].vramEnd);
						
						if (id == 1) { // PLAYER
							Mips64_SplitLoad(
								SegmentedToVirtual(0x0, romOff->table.player.init.hi),
								SegmentedToVirtual(0x0, romOff->table.player.init.lo),
								MIPS_REG_A0,
								Config_GetInt(&config, "init")
							);
							Mips64_SplitLoad(
								SegmentedToVirtual(0x0, romOff->table.player.dest.hi),
								SegmentedToVirtual(0x0, romOff->table.player.dest.lo),
								MIPS_REG_A0,
								Config_GetInt(&config, "dest")
							);
							Mips64_SplitLoad(
								SegmentedToVirtual(0x0, romOff->table.player.updt.hi),
								SegmentedToVirtual(0x0, romOff->table.player.updt.lo),
								MIPS_REG_A0,
								Config_GetInt(&config, "updt")
							);
							Mips64_SplitLoad(
								SegmentedToVirtual(0x0, romOff->table.player.draw.hi),
								SegmentedToVirtual(0x0, romOff->table.player.draw.lo),
								MIPS_REG_A0,
								Config_GetInt(&config, "draw")
							);
						} else { // PAUSE_MENU
							Mips64_SplitLoad(
								SegmentedToVirtual(0x0, romOff->table.pauseMenu.init.hi),
								SegmentedToVirtual(0x0, romOff->table.pauseMenu.init.lo),
								MIPS_REG_A0,
								Config_GetInt(&config, "init")
							);
							Mips64_SplitLoad(
								SegmentedToVirtual(0x0, romOff->table.pauseMenu.updt.hi),
								SegmentedToVirtual(0x0, romOff->table.pauseMenu.updt.lo),
								MIPS_REG_A0,
								Config_GetInt(&config, "updt")
							);
						}
						
						Dir_Leave(&gDir);
					}
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "skybox/"); {
			ItemList skyList;
			
			Rom_ItemList(&skyList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < skyList.num; i++) {
				if (skyList.item[i] == NULL)
					continue;
				
				printf_progress("Skybox", i + 1, skyList.num);
				
				Dir_Enter(&gDir, skyList.item[i]); {
					u32 texId = 941 + i * 2;
					u32 palId = 942 + i * 2;
					
					MemFile_Reset(&dataFile);
					MemFile_LoadFile(&dataFile, Dir_File(&gDir, "texel.bin"));
					Dma_WriteEntry(rom, texId, &dataFile, false);
					
					MemFile_Reset(&dataFile);
					MemFile_LoadFile(&dataFile, Dir_File(&gDir, "palette.bin"));
					Dma_WriteEntry(rom, palId, &dataFile, false);
					
					Dir_Leave(&gDir);
				}
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Enter(&gDir, "static/"); {
			ItemList statItem;
			
			Rom_ItemList(&statItem, SORT_NO, IS_FILE);
			
			for (s32 item = 0; item < statItem.num; item++) {
				s32 dmaId;
				printf_progress("Link Static", item + 1, statItem.num);
				
				for (dmaId = 0; dmaId <= ArrayCount(gSystemName_OoT); dmaId++) {
					if (dmaId == ArrayCount(gSystemName_OoT)) {
						dmaId = -1;
						break;
					}
					if (StrStrCase(statItem.item[item], gSystemName_OoT[dmaId]))
						break;
				}
				
				if (dmaId < 0) continue;
				if (dmaId == DMA_ID_CODE) continue;
				if (dmaId >= 36) dmaId += 901;
				
#if 1                          // Only these are supported atm
				if (dmaId != DMA_ID_TITLE_STATIC &&
					dmaId != DMA_ID_LINK_ANIMATION &&
					dmaId != DMA_ID_PARAMETER_STATIC
				)
					continue;
#endif
				
				MemFile_Reset(&dataFile);
				MemFile_LoadFile(&dataFile, Dir_File(&gDir, statItem.item[item]));
				
				Dma_WriteEntry(rom, dmaId, &dataFile, false);
			}
			
			Dir_Leave(&gDir);
		}
		
		Dir_Leave(&gDir);
	}
	
	if (Dir_Stat(&gDir, "patch/")) {
		Dir_Enter(&gDir, "patch/"); {
			printf_info("Applying Patches");
			Rom_Build_Patch(rom, &dataFile, &config);
			
			Dir_Leave(&gDir);
		}
	}
	
	Dir_Enter(&gDir, "rom/"); {
		if (Dir_Stat(&gDir, "lib_code/")) {
			Dir_Enter(&gDir, "lib_code/"); {
				Rom_Build_Code(rom, &dataFile, &config);
				
				Dir_Leave(&gDir);
			}
		}
		
		if (Dir_Stat(&gDir, "lib_user/")) {
			Dir_Enter(&gDir, "lib_user/"); {
				if (Dir_Stat(&gDir, "z_lib_user.bin")) {
					MemFile_Reset(&dataFile);
					MemFile_LoadFile(&dataFile, Dir_File(&gDir, "z_lib_user.bin"));
					Dma_WriteEntry(rom, DMA_ID_UNUSED_3, &dataFile, false);
				}
				
				Dir_Leave(&gDir);
			}
		}
		
		Dir_Leave(&gDir);
	}
	
	if (gCompressFlag) {
		Dma_PrintfSlots(rom);
		
		rom->file.dataSize = Dma_GetRomSize();
	}
	
	fix_crc(rom->file.data);
	MemFile_SaveFile(&rom->file, "build.z64");
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

void Rom_New(Rom* rom, char* romName) {
	u32* hdr;
	u16* addr;
	
	rom->file = MemFile_Initialize();
	MemFile_Params(&rom->file, MEM_FILENAME, true, MEM_END);
	
	if (MemFile_LoadFile(&rom->file, romName)) {
		printf_error_align("Error Opening", "%s", romName);
	}
	
	if (gInfoFlag == false && rom->file.dataSize < MbToBin(64)) {
		printf_warning_align("Rom Size", "[%.2fMb / %.2fMb]", BinToMb(rom->file.dataSize), 64.0f);
		printf_warning("Bad things " PRNT_REDD "might " PRNT_RSET "happen...");
		printf_warning("Do you want to continue the process? [y/n]");
		
		if (!printf_get_answer()) {
			exit(0);
		}
	}
	
	SetSegment(0x0, rom->file.data);
	hdr = SegmentedToVirtual(0x0, 0xDB70);
	
	if (rom->type == NoRom) {
		char* confRom = Tmp_Printf("%s%s", Sys_AppDir(), "z64project.cfg");
		MemFile* config = &rom->config;
		
		rom->config.seekPoint = strlen(rom->config.data);
		
		if (!Config_Get(&rom->config, "z_rom_type")) {
			if (hdr[0] != 0) {
				rom->type = Zelda_OoT_Debug;
				Config_WriteVar_Str("z_rom_type", "oot_debug # [oot_debug/oot_u10]");
			} else {
				rom->type = Zelda_OoT_1_0;
				Config_WriteVar_Str("z_rom_type", "oot_u10 # [oot_debug/oot_u10]");
			}
			MemFile_SaveFile_String(&rom->config, confRom);
		} else {
			char* romType = Config_GetString(&rom->config, "z_rom_type");
			
			if (!strcmp(romType, "oot_debug")) {
				rom->type = Zelda_OoT_Debug;
			} else if (!strcmp(romType, "oot_u10")) {
				rom->type = Zelda_OoT_1_0;
			} else {
				rom->type = NoRom;
			}
		}
		MemFile_Free(&rom->config);
	}
	
	switch (rom->type) {
		case Zelda_OoT_Debug:
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
			
			rom->offset.table.restrictionFlags = 0x00B9CA10;
			
			addr = SegmentedToVirtual(0x0, 0xB5A4AE);
			rom->offset.segment.seqRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
			rom->offset.segment.seqRom |= ReadBE(addr[2]);
			// rom->offset.segment.seqRom = 0x44DF0;
			
			addr = SegmentedToVirtual(0x0, 0xB5A4C2);
			rom->offset.segment.fontRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
			rom->offset.segment.fontRom |= ReadBE(addr[2]);
			// rom->offset.segment.fontRom = 0x19030;
			
			addr = SegmentedToVirtual(0x0, 0xB5A4D6);
			rom->offset.segment.smplRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
			rom->offset.segment.smplRom |= ReadBE(addr[2]);
			// rom->offset.segment.smplRom = 0x94870;
			
			rom->table.num.dma = 1548;
			rom->table.num.obj = 402;
			rom->table.num.actor = 471;
			rom->table.num.effect = 37;
			rom->table.num.state = 6;
			rom->table.num.scene = 110;
			rom->table.num.kaleido = 2;
			
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
			
			rom->offset.table.pauseMenu.init = (HiLo) {
				0x00B33208, 0x00B3320C
			};
			rom->offset.table.pauseMenu.updt = (HiLo) {
				0x00B33218, 0x00B33220
			};
			break;
		case Zelda_OoT_1_0:
			rom->offset.table.dmaTable = 0x00007430;
			rom->offset.table.objTable = 0x00B6EF58;
			rom->offset.table.actorTable = 0x00B5E490;
			rom->offset.table.effectTable = 0x00B5DBA0;
			rom->offset.table.stateTable = 0x00B672A0;
			rom->offset.table.sceneTable = 0x00B71440;
			rom->offset.table.kaleidoTable = 0x00B743E0;
			
			rom->offset.table.seqFontTbl = 0x00B89910;
			
			rom->offset.table.seqTable = 0x00B89AD0;
			rom->offset.table.fontTable = 0x00B896A0;
			rom->offset.table.sampleTable = 0x00B8A1C0;
			
			rom->offset.segment.seqRom = 0x00029DE0;
			rom->offset.segment.fontRom = 0x0000D390;
			rom->offset.segment.smplRom = 0x00079470;
			
			rom->offset.table.restrictionFlags = 0x00B6D2B0;
			
			rom->table.num.dma = 1526;
			rom->table.num.obj = 402;
			rom->table.num.actor = 471;
			rom->table.num.effect = 37;
			rom->table.num.state = 6;
			rom->table.num.scene = 101;
			rom->table.num.kaleido = 2;
			
			rom->offset.table.player.init = (HiLo) {
				0x00B0D5B8, 0x00B0D5C0
			};
			rom->offset.table.player.dest = (HiLo) {
				0x00B0D5C8, 0x00B0D5D4
			};
			rom->offset.table.player.updt = (HiLo) {
				0x00B0D5DC, 0x00B0D5E8
			};
			rom->offset.table.player.draw = (HiLo) {
				0x00B0D5F0, 0x00B0D5FC
			};
			break;
		case Zelda_MM_U:
			break;
		case NoRom:
		default:
			printf_error("Unknown ROM type");
	}
	
	rom->table.dma = SegmentedToVirtual(0x0, rom->offset.table.dmaTable);
	rom->table.object = SegmentedToVirtual(0x0, rom->offset.table.objTable);
	rom->table.actor = SegmentedToVirtual(0x0, rom->offset.table.actorTable);
	rom->table.effect = SegmentedToVirtual(0x0, rom->offset.table.effectTable);
	rom->table.state = SegmentedToVirtual(0x0, rom->offset.table.stateTable);
	rom->table.scene = SegmentedToVirtual(0x0, rom->offset.table.sceneTable);
	rom->table.kaleido = SegmentedToVirtual(0x0, rom->offset.table.kaleidoTable);
	
	rom->table.restrictionFlags = SegmentedToVirtual(0x0, rom->offset.table.restrictionFlags);
	
	rom->mem.sampleTbl = MemFile_Initialize();
	rom->mem.fontTbl = MemFile_Initialize();
	rom->mem.seqTbl = MemFile_Initialize();
	rom->mem.seqFontTbl = MemFile_Initialize();
	MemFile_Malloc(&rom->mem.sampleTbl, MbToBin(0.1));
	MemFile_Malloc(&rom->mem.fontTbl, MbToBin(0.1));
	MemFile_Malloc(&rom->mem.seqTbl, MbToBin(0.1));
	MemFile_Malloc(&rom->mem.seqFontTbl, MbToBin(0.1));
	
	if (rom->offset.segment.seqRom == 0x03F00000 &&
		rom->offset.segment.fontRom == 0x03E00000 &&
		rom->offset.segment.smplRom == 0x00094870 &&
		rom->type == Zelda_OoT_Debug) {
		u32* checkVal = SegmentedToVirtual(0x0, 0xBCC920);
		
		if (ReadBE(checkVal[0]) == 0x52059 && ReadBE(checkVal[1]) == 0x37F7) {
			SwapBE(rom->offset.table.seqTable);
		}
	}
}

void Rom_Free(Rom* rom) {
	MemFile_Free(&rom->file);
	MemFile_Free(&rom->config);
	MemFile_Free(&rom->mem.sampleTbl);
	MemFile_Free(&rom->mem.fontTbl);
	MemFile_Free(&rom->mem.seqTbl);
	MemFile_Free(&rom->mem.seqFontTbl);
	memset(rom, 0, sizeof(struct Rom));
}

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

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