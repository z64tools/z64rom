#include "z64rom.h"

static u32 Rom_Ovl_GetBssSize(MemFile* dataFile) {
	u32* bssSize;
	
	SetSegment(0x1, dataFile->data);
	bssSize = SegmentedToVirtual(0x1, dataFile->dataSize - 4);
	bssSize = SegmentedToVirtual(0x1, dataFile->dataSize - ReadBE(bssSize[0]));
	
	return ReadBE(bssSize[3]);
}

static RestrictionFlag* Rom_GetRestrictionFlags(Rom* rom, u32 sceneIndex) {
	RestrictionFlag* flagList = rom->table.restrictionFlags;
	
	while (flagList->sceneIndex != 0xFF) {
		if (flagList->sceneIndex == sceneIndex)
			return flagList;
		
		flagList++;
	}
	
	return NULL;
}

static void Rom_WriteRestrictionFlags(Rom* rom, MemFile* config, u32 sceneIndex) {
	char* flags = Config_GetVariable(config->str, "restriction_flags");
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

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

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

// # # # # # # # # # # # # # # # # # # # #
// # PATCH                               #
// # # # # # # # # # # # # # # # # # # # #

typedef struct PatchNode {
	struct PatchNode* prev;
	struct PatchNode* next;
	u32  start;
	u32  end;
	char source[64];
} PatchNode;

PatchNode* sPatchHead;

static void Rom_Patch_Config(Rom* rom, MemFile* dataFile, MemFile* config, char* file) {
	u32 lineCount;
	char* line;
	
	MemFile_Reset(config);
	Log("Loading Patch [%s]", file);
	MemFile_LoadFile_String(config, file);
	MemFile_Realloc(config, config->memSize * 2);
	
	line = config->str;
	lineCount = String_LineNum(config->str);
	
	for (s32 i = 0; i < lineCount; i++, line = String_Line(line, 1)) {
		if (line == NULL)
			break;
		if (line[0] != '@')
			continue;
		
		char* cmd = String_GetWord(line, 0);
		
		String_Replace(config->str, cmd + 1, Config_GetVariable(line, cmd));
	}
	
	line = config->str;
	
	for (s32 i = 0; i < lineCount; i++, line = String_Line(line, 1)) {
		PatchNode* node;
		char* word;
		if (line == NULL)
			break;
		if (line[0] == '@')
			continue;
		
		if (String_Validate_Hex(String_GetWord(line, 0)) == false)
			continue;
		
		word = Config_Variable(line, String_GetWord(line, 0));
		rom->file.seekPoint = String_GetHexInt(String_GetWord(line, 0));
		
		if (StrMtch(word, "FILE(\"")) {
			word = Config_GetVariable(line, String_GetWord(line, 0));
			
			// Config_GetVariable sees that this is a string and does magics
			String_Remove(word, strlen("ILE(\""));
			
			Sys_SetWorkDir(String_GetPath(file));
			if (Sys_Stat(word)) {
				MemFile ptch = MemFile_Initialize();
				
				if (MemFile_LoadFile(&ptch, word))
					printf_error("Could not open a file that should exist [%s]", word);
				
				node = Calloc(node, sizeof(struct PatchNode));
				node->start = rom->file.seekPoint;
				node->end = rom->file.seekPoint + strlen(word);
				strncpy(node->source, file, 63);
				Node_Add(sPatchHead, node);
				
				MemFile_Append(&rom->file, &ptch);
			} else {
				printf_warning("Could not locate patch file [%s] provided by [%s]", word, file);
			}
			Sys_SetWorkDir(Sys_AppDir());
			
			continue;
		}
		
		if (word[0] == '"') {
			word = Config_GetVariable(line, String_GetWord(line, 0));
			
			node = Calloc(node, sizeof(struct PatchNode));
			node->start = rom->file.seekPoint;
			node->end = rom->file.seekPoint + strlen(word);
			strncpy(node->source, file, 63);
			Node_Add(sPatchHead, node);
			
			Log("STR: 0x%08X = \"%s\"", rom->file.seekPoint, word);
			MemFile_Printf(&rom->file, word);
			
			continue;
		}
		
		word = Config_GetVariable(line, String_GetWord(line, 0));
		
		if (String_Validate_Hex(word)) {
			u8* data = &rom->file.cast.u8[rom->file.seekPoint];
			u32 size = 0;
			
			Log("HEX: 0x%08X = %s", rom->file.seekPoint, word);
			
			for (s32 o = 0, j = word[1] == 'x' ? 2 : 0; j < strlen(word); j++) {
				u8 new;
				char strval[] = {
					word[j],
					'\0'
				};
				
				if (word[j] == '#' || word[j] < ' ' || word[j] == '\0')
					break;
				
				if (word[j] == ' ' || word[j] == '\t' || (word[j] == '0' && word[j + 1] == 'x') || word[j] == 'x')
					continue;
				
				if (word[j] == '.') {
					if (o % 2 != 0) {
						data++;
						size++;
					}
				} else {
					if (o % 2 == 0) {
						new = data[0] & 0x0F;
						new |= String_GetHexInt(strval) << 4;
						data[0] = new;
					} else {
						new = data[0] & 0xF0;
						new |= String_GetHexInt(strval) & 0xF;
						data[0] = new;
						data++;
						size++;
					}
				}
				
				o++;
			}
			
			node = Calloc(node, sizeof(struct PatchNode));
			node->start = rom->file.seekPoint;
			node->end = rom->file.seekPoint + size;
			strncpy(node->source, file, 63);
			Node_Add(sPatchHead, node);
		}
	}
}

static void Rom_Build_Patch(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList list = ItemList_Initialize();
	
	ItemList_List(&list, "patch/", -1, LIST_FILES);
	
	for (s32 i = 0; i < list.num; i++) {
		if (StrEndCase(list.item[i], ".cfg"))
			Rom_Patch_Config(rom, dataFile, config, list.item[i]);
	}
}

static void Rom_Build_Code(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList list = ItemList_Initialize();
	
	Dir_ItemList_Recursive(&list, ".bin");
	
	for (s32 i = 0; i < list.num; i++) {
		PatchNode* node = sPatchHead;
		u32 nodeID = 0;
		char* fileCfg = Dir_File("%s%s.cfg", String_GetPath(list.item[i]), String_GetBasename(list.item[i]));
		
		MemFile_Reset(dataFile);
		MemFile_Reset(config);
		
		if (!Sys_Stat(fileCfg))
			printf_error("Could not find [%s]", fileCfg);
		
		if (MemFile_LoadFile(dataFile, Dir_File(list.item[i]))) printf_error("Exiting...");
		if (MemFile_LoadFile_String(config, fileCfg)) printf_error("Exiting...");
		
		rom->file.seekPoint = Config_GetInt(config, "rom");
		
		while (node) {
			if (Intersect(node->start, node->end, rom->file.seekPoint, rom->file.seekPoint + dataFile->dataSize)) {
				printf_warning(
					"Patch to "
					"[" PRNT_REDD "0x%08X" PRNT_RSET "]"
					" from "
					"[" PRNT_REDD "patch/%s" PRNT_RSET "]"
					" has been overwritten by "
					"[" PRNT_REDD "rom/lib_code/%s" PRNT_RSET "]!",
					node->start,
					node->source,
					list.item[i]
				);
			}
			
			nodeID++;
			node = node->next;
		}
		
		MemFile_Append(&rom->file, dataFile);
	}
	
	ItemList_Free(&list);
	
	while (sPatchHead)
		Node_Kill(sPatchHead, sPatchHead);
}

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

void Rom_Dump(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	RomFile rf;
	
	Dir_SetParam(DIR__MAKE_ON_ENTER);
	MemFile_Malloc(&dataFile, 0x460000); // Slightly larger than audiotable
	MemFile_Malloc(&config, 0x25000);
	
	printf_info_align("Dumping Rom", PRNT_REDD "%s", String_GetFilename(rom->file.info.name));
	
	Dir_Enter("rom/"); {
		Dir_Enter("actor/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.actor; i++) {
				rf = Dma_RomFile_Actor(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Actor", i + 1, rom->table.num.actor);
				Dir_Enter("0x%04X-%s/", i, gActorName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File("actor.zovl")))
						Rom_Config_Actor(&config, &rom->table.actor[i], gActorName_OoT[i], Dir_File("config.cfg"));
					
					Dir_Leave();
				}
			}
			
			Dir_Leave();
		}
		
		Dir_Enter("effect/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.effect; i++) {
				rf = Dma_RomFile_Effect(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Effect", i + 1, rom->table.num.effect);
				Dir_Enter("0x%04X-%s/", i, gEffectName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File("effect.zovl")))
						Rom_Config_Effect(&config, &rom->table.effect[i], gEffectName_OoT[i], Dir_File("config.cfg"));
					
					Dir_Leave();
				}
			}
			
			Dir_Leave();
		}
		
		Dir_Enter("object/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.obj; i++) {
				rf = Dma_RomFile_Object(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Object", i + 1, rom->table.num.obj);
				Dir_Enter("0x%04X-%s/", i, gObjectName_OoT[i]); {
					Rom_Extract(&dataFile, rf, Dir_File("object.zobj"));
					
					Dir_Leave();
				}
			}
			
			Dir_Leave();
		}
		
		Dir_Enter("scene/.vanilla/"); {
			for (s32 i = 0; i < rom->table.num.scene; i++) {
				SceneEntry* scene = &rom->table.scene[i];
				RomFile png;
				rf = Dma_RomFile_Scene(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Scene", i + 1, rom->table.num.scene);
				Dir_Enter("0x%02X-%s/", i, gSceneName_OoT[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File("scene.zscene"))) {
						u32* seg;
						u32 roomNum;
						u32 roomListSeg;
						u32* vromSeg;
						
						Rom_Config_Scene(rom, &config, i, gSceneName_OoT[i], Dir_File("config.cfg"));
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
								char* out = Dir_File("room_%d.zroom", j);
								
								vromSeg = SegmentedToVirtual(0x2, roomListSeg + 8 * j);
								Rom_Extract(
									&dataFile,
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
							Dir_File("title.png"),
							TEX_FMT_IA,
							TEX_BSIZE_8,
							144,
							24
						);
					}
					
					Dir_Leave();
				}
			}
			
			Dir_Leave();
		}
		
		Dir_Enter("system/"); {
			Dir_Enter("state/.vanilla/"); {
				for (s32 i = 0; i < rom->table.num.state; i++) {
					rf = Dma_RomFile_GameState(rom, i);
					
					if (rf.size == 0)
						continue;
					
					printf_progress("System", i + 1, rom->table.num.state);
					Dir_Enter("0x%02X-%s/", i, gStateName_OoT[i]); {
						if (Rom_Extract(&dataFile, rf, Dir_File("state.zovl")))
							Rom_Config_GameState(&config, &rom->table.state[i], gStateName_OoT[i], Dir_File("config.cfg"));
						
						Dir_Leave();
					}
				}
				Dir_Leave();
			}
			
			Dir_Enter("kaleido/.vanilla/"); {
				for (s32 i = 0; i < rom->table.num.kaleido; i++) {
					Dir_Enter("0x%02X-%s/", i, gKaleidoName_OoT[i]); {
						rf.size = ReadBE(rom->table.kaleido[i].vromEnd) - ReadBE(rom->table.kaleido[i].vromStart);
						rf.data = SegmentedToVirtual(0x0, ReadBE(rom->table.kaleido[i].vromStart));
						
						Rom_Extract(&dataFile, rf, Dir_File("overlay.zovl"));
						Rom_Config_Kaleido(rom, &config, i, gKaleidoName_OoT[i], Dir_File("config.cfg"));
						
						Dir_Leave();
					}
				}
				Dir_Leave();
			}
			
			Dir_Enter("static/.vanilla/"); {
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
					
					Rom_Extract(&dataFile, rf, Dir_File("%s.bin", gSystemName_OoT[name]));
				}
				
				Dir_Leave();
			}
			
			Dir_Enter("skybox/.vanilla/"); {
				for (s32 i = 0; i < 32; i++) {
					printf_progress("Skybox", i + 1, 32);
					
					Dir_Enter("%02d-%s/", i, gSkyboxName_OoT[i]); {
						rf.romStart = ReadBE(rom->table.dma[941 + i * 2].vromStart);
						rf.romEnd = ReadBE(rom->table.dma[941 + i * 2].vromEnd);
						rf.size = rf.romEnd - rf.romStart;
						rf.data = SegmentedToVirtual(0x0, rf.romStart);
						Rom_Extract(&dataFile, rf, Dir_File("texel.bin"));
						
						rf.romStart = ReadBE(rom->table.dma[942 + i * 2].vromStart);
						rf.romEnd = ReadBE(rom->table.dma[942 + i * 2].vromEnd);
						rf.size = rf.romEnd - rf.romStart;
						rf.data = SegmentedToVirtual(0x0, rf.romStart);
						Rom_Extract(&dataFile, rf, Dir_File("palette.bin"));
						
						Dir_Leave();
					}
				}
				
				Dir_Leave();
			}
			
			Dir_Leave();
		}
		
		Dir_Enter("sound/"); {
			Rom_Dump_SoundFont(rom, &dataFile, &config);
			Rom_Dump_Sequences(rom, &dataFile, &config);
			Rom_Dump_Samples(rom, &dataFile, &config);
			
			Dir_Leave();
		}
		
		Dir_Leave();
	}
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

static void Rom_ExtTableNum(Rom* rom) {
	MemFile ulib = MemFile_Initialize();
	char* fname = "src/lib_user/uLib.h";
	char* word;
	
	if (MemFile_LoadFile_String(&ulib, fname))
		return;
	
	word = StrStr(ulib.str, "EXT_DMA_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_DMA_MAX", fname);
	rom->ext.dmaNum = String_GetInt(String_Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_ACTOR_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_ACTOR_MAX", fname);
	rom->ext.actorNum = String_GetInt(String_Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_OBJECT_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_OBJECT_MAX", fname);
	rom->ext.objectNum = String_GetInt(String_Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_SCENE_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_SCENE_MAX", fname);
	rom->ext.sceneNum = String_GetInt(String_Word(word, 1));
	
	word = StrStr(ulib.str, "EXT_EFFECT_MAX");
	if (word == NULL) printf_error("Could not find [%s] in [%s]", "EXT_EFFECT_MAX", fname);
	rom->ext.effectNum = String_GetInt(String_Word(word, 1));
	
	Log("DmaNum: %d", rom->ext.dmaNum);
	Log("ActorNum: %d", rom->ext.actorNum);
	Log("ObjectNum: %d", rom->ext.objectNum);
	Log("SceneNum: %d", rom->ext.sceneNum);
	Log("EffectNum: %d", rom->ext.effectNum);
	
	MemFile_Free(&ulib);
	
	Dma_FreeEntry(rom, DMA_ID_UNUSED_2, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_2, false);
}

#define Rom_MoveTable(OFFSET, O_TABLE, NUM, NEW_NUM) \
	OFFSET = rom->offset.table.dmaTable + size; \
	table = SegmentedToVirtual(0, OFFSET); \
	memcpy(table, O_TABLE, sizeof(*O_TABLE) * NUM); \
	O_TABLE = table; \
	NUM = NEW_NUM; \
	size += sizeof(*O_TABLE) * NEW_NUM; \
	size = Align(size, 16);

static void Rom_AllocDmaTable(Rom* rom) {
	u32 size = 0;
	void* table;
	
	if (rom->ext.dmaNum == 0)
		return;
	
	size += sizeof(struct DmaEntry) * rom->ext.dmaNum;
	size = Align(size, 16);
	
	Rom_MoveTable(rom->offset.table.actorTable, rom->table.actor, rom->table.num.actor, rom->ext.actorNum);
	
	size += sizeof(struct ObjectEntry) * rom->ext.objectNum;
	size = Align(size, 16);
	
	Rom_MoveTable(rom->offset.table.sceneTable, rom->table.scene, rom->table.num.scene, rom->ext.sceneNum);
	Rom_MoveTable(rom->offset.table.effectTable, rom->table.effect, rom->table.num.effect, rom->ext.effectNum);
	
	if (rom->offset.table.dmaTable != Dma_AllocEntry(rom, 2, size))
		printf_error("Tables have turned!");
	
	for (s32 i = rom->table.num.dma; i < rom->ext.dmaNum; i++) {
		if (i == rom->ext.dmaNum - 1)
			rom->table.dma[i] = (DmaEntry) { 0 };
		
		else
			rom->table.dma[i] = (DmaEntry) { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	}
}

void Rom_Build(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	
	MemFile_Malloc(&dataFile, 0x460000);
	MemFile_Malloc(&config, 0x25000);
	
	MemFile_Params(&dataFile, MEM_REALLOC, true, MEM_END);
	MemFile_Params(&config, MEM_REALLOC, true, MEM_END);
	
	printf_info_align("Load Baserom", PRNT_REDD "%s", String_GetFilename(rom->file.info.name));
	printf_info_align("Build Rom", PRNT_BLUE "%s",  gRomName_Output[gBuildTarget]);
	
	Rom_ExtTableNum(rom);
	
	Dma_FreeEntry(rom, DMA_ID_UNUSED_3, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_3, false);
	Dma_FreeEntry(rom, DMA_ID_UNUSED_4, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_4, false);
	Dma_FreeEntry(rom, DMA_ID_UNUSED_5, 0x10); Dma_WriteFlag(DMA_ID_UNUSED_5, false);
	
	Dma_FreeEntry(rom, DMA_ID_LINK_ANIMATION, 0x1000); Dma_WriteFlag(DMA_ID_LINK_ANIMATION, false);
	Dma_FreeEntry(rom, DMA_ID_TITLE_STATIC, 0x1000); Dma_WriteFlag(DMA_ID_TITLE_STATIC, false);
	Dma_FreeEntry(rom, DMA_ID_PARAMETER_STATIC, 0x1000); Dma_WriteFlag(DMA_ID_PARAMETER_STATIC, false);
	
	Dma_FreeGroup(rom, DMA_ACTOR);
	Dma_FreeGroup(rom, DMA_STATE);
	Dma_FreeGroup(rom, DMA_KALEIDO);
	Dma_FreeGroup(rom, DMA_EFFECT);
	Dma_FreeGroup(rom, DMA_OBJECT);
	Dma_FreeGroup(rom, DMA_SCENES);
	Dma_FreeGroup(rom, DMA_PLACE_NAME);
	Dma_FreeGroup(rom, DMA_SKYBOX_TEXEL);
	Dma_FreeGroup(rom, DMA_UNUSED);
	
	Dma_CombineSlots();
	
	if (gPrintInfo)
		Dma_PrintfSlots(rom, "Marked Free");
	
	Rom_AllocDmaTable(rom);
	
	Dir_Enter("rom/"); {
		Dir_Enter("sound/"); {
			Dir_Enter("sample/"); {
				Rom_Build_SampleTable(rom, &dataFile, &config);
				
				Dir_Leave();
			}
			Dir_Enter("soundfont/"); {
				Rom_Build_SoundFont(rom, &dataFile, &config);
				
				Dir_Leave();
			}
			Dir_Enter("sequence/"); {
				Rom_Build_Sequence(rom, &dataFile, &config);
				
				Dir_Leave();
			}
			Rom_Build_SetAudioSegment(rom);
			
			Dir_Leave();
		}
		
		Dir_Enter("actor/"); {
			ItemList actorList = ItemList_Initialize();
			ActorEntry* entry = rom->table.actor;
			Rom_ItemList(&actorList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < actorList.num; i++) {
				if (actorList.item[i] == NULL) {
					if (entry[i].vromStart && entry[i].vromEnd)
						entry[i] = (ActorEntry) { 0 };
					continue;
				}
				
				if (i >= rom->table.num.actor) {
					printf_warning("ActorTable Full [%d/%d]", i + 1, rom->table.num.actor);
					break;
				} else
					printf_progress("Actor", i + 1, actorList.num);
				
				Dir_Enter(actorList.item[i]); {
					char* ovl = Dir_File("*.zovl");
					
					MemFile_Reset(&dataFile);
					MemFile_Reset(&config);
					MemFile_LoadFile(&dataFile, ovl);
					MemFile_LoadFile_String(&config, Dir_File("config.cfg"));
					
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
					
					if (gBuildTarget == ROM_DEV && entry[i].name) {
						u32 romAddr = ReadBE(entry[i].name) - 0x7F588E60;
						void* target;
						char buf[64];
						u32 id;
						
						Log("Filename Offset [%08X]", romAddr);
						
						target = SegmentedToVirtual(0, romAddr);
						ovl = String_GetFolder(actorList.item[i], -1);
						sscanf(ovl, "0x%04X-%s/", &id, buf);
						strncpy(target, buf, strlen(target));
					}
					
					Dir_Leave();
				}
			}
			
			ItemList_Free(&actorList);
			Dir_Leave();
		}
		
		Dir_Enter("effect/"); {
			ItemList effectList = ItemList_Initialize();
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
				
				Dir_Enter(effectList.item[i]); {
					MemFile_Reset(&dataFile);
					MemFile_Reset(&config);
					MemFile_LoadFile(&dataFile, Dir_File("*.zovl"));
					MemFile_LoadFile_String(&config, Dir_File("config.cfg"));
					
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
					
					Dir_Leave();
				}
			}
			
			ItemList_Free(&effectList);
			Dir_Leave();
		}
		
		Dir_Enter("object/"); {
			ItemList objectList = ItemList_Initialize();
			ObjectEntry* entry = rom->table.object;
			Rom_ItemList(&objectList, SORT_NUMERICAL, IS_DIR);
			
			for (s32 i = 0; i < objectList.num; i++) {
				if (objectList.item[i] == NULL) {
					entry[i].vromStart = 0;
					entry[i].vromEnd = 0;
					
					continue;
				}
				
				Dir_Enter(objectList.item[i]); {
					printf_progress("Object", i + 1, objectList.num);
					
					MemFile_Reset(&dataFile);
					MemFile_LoadFile(&dataFile, Dir_File("*.zobj"));
					
					entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, true);
					entry[i].vromEnd = Dma_GetVRomEnd();
					SwapBE(entry[i].vromStart);
					SwapBE(entry[i].vromEnd);
					
					Dir_Leave();
				}
			}
			
			ItemList_Free(&objectList);
			Dir_Leave();
		}
		
		Dir_Enter("scene/"); {
			MemFile memRoom = MemFile_Initialize();
			ItemList sceneList = ItemList_Initialize();
			ItemList titleList = ItemList_Initialize();
			SceneEntry* entry = rom->table.scene;
			u32* titleID;
			
			MemFile_Malloc(&memRoom, MbToBin(2));
			Rom_ItemList(&sceneList, SORT_NUMERICAL, IS_DIR);
			
			ItemList_Alloc(&titleList, sceneList.num, sceneList.writePoint * 4);
			titleID = Calloc(titleID, sizeof(u32) * sceneList.num);
			
			for (s32 i = 0; i < sceneList.num; i++) {
				if (sceneList.item[i] == NULL) {
					printf_error("Empty scene %d", i);
				}
				
				Dir_Enter(sceneList.item[i]); {
					u32* seg;
					u32* vromSeg;
					u32 roomNum;
					u32 roomListSeg;
					
					printf_progress("Scene", i + 1, sceneList.num);
					
					MemFile_Reset(&config);
					MemFile_Reset(&dataFile);
					if (MemFile_LoadFile_String(&config, Dir_File("config.cfg"))) printf_error("Exiting...");
					if (MemFile_LoadFile(&dataFile, Dir_File("scene.zscene"))) printf_error("Exiting...");
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
						if (Sys_Stat(Dir_File("room_%d.zroom", j))) {
							if (MemFile_LoadFile(&memRoom, Dir_File("room_%d.zroom", j)))
								printf_error("Exiting...");
						} else if (MemFile_LoadFile(&memRoom, Dir_File("room_%d.zmap", j)))
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
					entry[i].titleVromStart = 0;
					entry[i].titleVromEnd = 0;
					
					if (Dir_Stat("title.png")) {
						titleID[titleList.num] = i;
						ItemList_AddItem(&titleList, Dir_File("title.png"));
					}
					
					Dir_Leave();
				}
			}
			
			// Check for unique Place Names
			for (s32 i = 0; i < titleList.num; i++) {
				u32 useSame = false;
				Texel_Import(&dataFile, titleList.item[i], TEX_FMT_IA, TEX_BSIZE_8, 144, 24);
				
				for (s32 j = 0; j < i; j++) {
					u32 id = titleID[j];
					void* data;
					Size sz;
					
					data = SegmentedToVirtual(0, ReadBE(entry[id].titleVromStart));
					sz = ReadBE(entry[id].titleVromEnd) - ReadBE(entry[id].titleVromStart);
					
					if (!memcmp(data, dataFile.data, sz)) {
						useSame = true;
						
						entry[titleID[i]].titleVromStart = entry[id].titleVromStart;
						entry[titleID[i]].titleVromEnd = entry[id].titleVromEnd;
					}
				}
				
				if (useSame == false) {
					entry[titleID[i]].titleVromStart = ReadBE(Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false));
					entry[titleID[i]].titleVromEnd = ReadBE(Dma_GetVRomEnd());
				}
			}
			
			MemFile_Free(&memRoom);
			ItemList_Free(&sceneList);
			ItemList_Free(&titleList);
			Free(titleID);
			Dir_Leave();
		}
		
		Dir_Enter("system/"); {
			Dir_Enter("state/"); {
				ItemList gameSysList = ItemList_Initialize();
				
				Rom_ItemList(&gameSysList, SORT_NUMERICAL, IS_DIR);
				
				for (s32 i = 0; i < gameSysList.num; i++) {
					printf_progress("System", i + 1, gameSysList.num);
					
					if (gameSysList.item[i] == NULL || strlen(gameSysList.item[i]) == 0)
						continue;
					
					Dir_Enter(gameSysList.item[i]); {
						GameStateEntry* entry = rom->table.state;
						MemFile_Reset(&config);
						MemFile_Reset(&dataFile);
						
						if (MemFile_LoadFile_String(&config, Dir_File("config.cfg")))
							printf_error("Exiting");
						if (MemFile_LoadFile(&dataFile, Dir_File("*.zovl")))
							printf_error("Exiting");
						
						entry[i].init = Config_GetInt(&config, "init_func");
						entry[i].destroy = Config_GetInt(&config, "dest_func");
						
						entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
						entry[i].vromEnd = Dma_GetVRomEnd();
						
						entry[i].vramStart = Config_GetInt(&config, "vram_addr");
						entry[i].vramEnd = entry[i].vramStart + dataFile.dataSize + Rom_Ovl_GetBssSize(&dataFile);
						
						SwapBE(entry[i].init);
						SwapBE(entry[i].destroy);
						SwapBE(entry[i].vromStart);
						SwapBE(entry[i].vromEnd);
						SwapBE(entry[i].vramStart);
						SwapBE(entry[i].vramEnd);
						
						Dir_Leave();
					}
				}
				
				ItemList_Free(&gameSysList);
				Dir_Leave();
			}
			
			Dir_Enter("kaleido/"); {
				ItemList kaleidoList = ItemList_Initialize();
				
				Rom_ItemList(&kaleidoList, SORT_NUMERICAL, IS_DIR);
				
				for (s32 i = 0; i < kaleidoList.num; i++) {
					printf_progress("Kaleido", i + 1, kaleidoList.num);
					
					Dir_Enter(kaleidoList.item[i]); {
						KaleidoEntry* entry = rom->table.kaleido;
						RomOffset* romOff = &rom->offset;
						MemFile_Reset(&config);
						MemFile_Reset(&dataFile);
						
						if (MemFile_LoadFile_String(&config, Dir_File("config.cfg")))
							printf_error("Exiting");
						if (MemFile_LoadFile(&dataFile, Dir_File("*.zovl")))
							printf_error("Exiting");
						
						entry[i].vromStart = Dma_WriteEntry(rom, DMA_FIND_FREE, &dataFile, false);
						entry[i].vromEnd = Dma_GetVRomEnd();
						
						entry[i].vramStart = Config_GetInt(&config, "vram_addr");
						entry[i].vramEnd = entry[i].vramStart + dataFile.dataSize + Rom_Ovl_GetBssSize(&dataFile);
						
						SwapBE(entry[i].vromStart);
						SwapBE(entry[i].vromEnd);
						SwapBE(entry[i].vramStart);
						SwapBE(entry[i].vramEnd);
						
						if (i == 1) { // PLAYER
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
						
						Dir_Leave();
					}
				}
				
				ItemList_Free(&kaleidoList);
				Dir_Leave();
			}
			
			Dir_Enter("skybox/"); {
				ItemList skyList = ItemList_Initialize();
				
				Rom_ItemList(&skyList, SORT_NUMERICAL, IS_DIR);
				
				for (s32 i = 0; i < skyList.num; i++) {
					if (skyList.item[i] == NULL)
						continue;
					
					printf_progress("Skybox", i + 1, skyList.num);
					
					Dir_Enter(skyList.item[i]); {
						u32 texId = 941 + i * 2;
						u32 palId = 942 + i * 2;
						
						MemFile_Reset(&dataFile);
						MemFile_LoadFile(&dataFile, Dir_File("texel.bin"));
						Dma_WriteEntry(rom, texId, &dataFile, false);
						
						MemFile_Reset(&dataFile);
						MemFile_LoadFile(&dataFile, Dir_File("palette.bin"));
						Dma_WriteEntry(rom, palId, &dataFile, false);
						
						Dir_Leave();
					}
				}
				
				ItemList_Free(&skyList);
				Dir_Leave();
			}
			
			Dir_Enter("static/"); {
				ItemList statItem = ItemList_Initialize();
				
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
					MemFile_LoadFile(&dataFile, Dir_File(statItem.item[item]));
					
					Dma_WriteEntry(rom, dmaId, &dataFile, false);
				}
				
				ItemList_Free(&statItem);
				Dir_Leave();
			}
			
			Dir_Leave();
		}
		
		Dir_Leave();
	}
	
	printf_info("Applying Patches");
	Rom_Build_Patch(rom, &dataFile, &config);
	
	Dir_Enter("rom/"); {
		if (Dir_Stat("lib_code/")) {
			Dir_Enter("lib_code/"); {
				Rom_Build_Code(rom, &dataFile, &config);
				
				Dir_Leave();
			}
		}
		
		if (Dir_Stat("lib_user/")) {
			Dir_Enter("lib_user/"); {
				if (Dir_Stat("z_lib_user.bin")) {
					MemFile_Reset(&dataFile);
					MemFile_LoadFile(&dataFile, Dir_File("z_lib_user.bin"));
					Dma_WriteEntry(rom, DMA_ID_UNUSED_3, &dataFile, false);
				}
				
				Dir_Leave();
			}
		}
		
		Dir_Leave();
	}
	
	if (gPrintInfo) {
		Dma_PrintfSlots(rom, "Left Free");
	}
	
	Slot* slot = gSlotHead;
	
	while (slot->next)
		slot = slot->next;
	
	rom->file.dataSize = slot->romStart;
	
	rom->file.dataSize = Align(rom->file.dataSize, MbToBin(1));
	
	fix_crc(rom->file.data);
	MemFile_SaveFile(&rom->file, gRomName_Output[gBuildTarget]);
	
	MemFile_Free(&dataFile);
	MemFile_Free(&config);
}

void Rom_New(Rom* rom, char* romName) {
	u32* hdr;
	u16* addr;
	
	rom->file = MemFile_Initialize();
	
	MemFile_Malloc(&rom->file, MbToBin(128));
	
	if (MemFile_LoadFile(&rom->file, romName)) {
		printf_error_align("Error Opening", "%s", romName);
	}
	
	SetSegment(0x0, rom->file.data);
	hdr = SegmentedToVirtual(0x0, 0xDB70);
	
	if (rom->type == NoRom) {
		char* romType = Config_GetVariable(rom->config.str, "z_rom_type");
		
		if (!strcmp(romType, "__PLACEHOLDER__")) {
			if (hdr[0] != 0) {
				rom->type = Zelda_OoT_Debug;
				String_Replace(rom->config.str, "__PLACEHOLDER__", "oot_debug");
			} else {
				rom->type = Zelda_OoT_1_0;
				String_Replace(rom->config.str, "__PLACEHOLDER__", "oot_u10");
			}
			rom->config.seekPoint = rom->config.dataSize = strlen(rom->config.str);
		} else {
			if (!strcmp(romType, "oot_debug")) {
				rom->type = Zelda_OoT_Debug;
			} else if (!strcmp(romType, "oot_u10")) {
				rom->type = Zelda_OoT_1_0;
			} else {
				rom->type = NoRom;
			}
		}
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

void Rom_DeleteUnusedContent(s32 romType) {
	ItemList list = ItemList_Initialize();
	char* item;
	u32 id;
	
	if (romType == Zelda_OoT_Debug) {
		ItemList_List(&list, "rom/actor/.vanilla/", 0, LIST_FOLDERS | LIST_RELATIVE);
		ItemList_NumericalSort(&list);
		for (s32 i = 0; i < ArrayCount(gBetaFlag_Actor_OoT); i++) {
			id = gBetaFlag_Actor_OoT[i];
			
			if (list.item[id] == NULL || id >= list.num)
				continue;
			
			item = Tmp_Printf("rom/actor/.vanilla/%s", list.item[id]);
			
			printf_info("Delete [%s]", item);
			Sys_Delete_Recursive(item);
		}
		ItemList_Free(&list);
		
		ItemList_List(&list, "rom/object/.vanilla/", 0, LIST_FOLDERS | LIST_RELATIVE);
		ItemList_NumericalSort(&list);
		for (s32 i = 0; i < ArrayCount(gBetaFlag_Object_OoT); i++) {
			id = gBetaFlag_Object_OoT[i];
			
			if (list.item[id] == NULL || id >= list.num)
				continue;
			
			item = Tmp_Printf("rom/object/.vanilla/%s", list.item[id]);
			
			printf_info("Delete [%s]", item);
			Sys_Delete_Recursive(item);
		}
		ItemList_Free(&list);
		
		ItemList_List(&list, "rom/scene/.vanilla/", 0, LIST_FOLDERS | LIST_RELATIVE);
		ItemList_NumericalSort(&list);
		for (s32 i = 0; i < ArrayCount(gBetaFlag_Scene_OoT); i++) {
			id = gBetaFlag_Scene_OoT[i];
			
			if (list.item[id] == NULL || id >= list.num)
				continue;
			
			item = Tmp_Printf("rom/scene/.vanilla/%s", list.item[id]);
			
			printf_info("Delete [%s]", item);
			Sys_Delete_Recursive(item);
		}
		ItemList_Free(&list);
	}
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

void Rom_ItemList(ItemList* list, bool isNum, bool isDir) {
	ItemList vanilla = ItemList_Initialize();
	ItemList modified = ItemList_Initialize();
	ItemList result = ItemList_Initialize();
	
	Dir_Enter(".vanilla/"); {
		Dir_ItemList(&vanilla, isDir);
		
		Dir_Leave();
	}
	Dir_ItemList(&modified, isDir);
	
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
	
	u32 size = 0;
	
	for (s32 i = 0; i < list->num; i++) {
		if (!list->item[i])
			continue;
		size += strlen(list->item[i]) + 1;
	}
	
	result.num = list->num;
	result.buffer = Calloc(result.buffer, size);
	result.item = Calloc(result.item, sizeof(u8*) * list->num);
	
	for (s32 i = 0; i < list->num; i++) {
		if (!list->item[i])
			continue;
		
		result.item[i] = &result.buffer[result.writePoint];
		strcpy(result.item[i], list->item[i]);
		result.writePoint += strlen(result.item[i]) + 1;
	}
	
	list[0] = result;
	
	ItemList_Free(&vanilla);
	ItemList_Free(&modified);
}

void Rom_ItemList_NDIR(ItemList* list, const char* path, bool isNum, ListFlags flags) {
	ItemList vanilla = ItemList_Initialize();
	ItemList modified = ItemList_Initialize();
	ItemList result = ItemList_Initialize();
	
	ItemList_List(&vanilla, Tmp_Printf("%s.vanilla/", path), 0, flags | LIST_RELATIVE);
	ItemList_List(&modified, path, 0, flags | LIST_NO_DOT | LIST_RELATIVE);
	
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
	
	u32 size = 0;
	
	for (s32 i = 0; i < list->num; i++) {
		if (!list->item[i])
			continue;
		size += strlen(list->item[i]) + 1;
	}
	
	result.num = list->num;
	result.buffer = Calloc(result.buffer, size);
	result.item = Calloc(result.item, sizeof(u8*) * list->num);
	
	for (s32 i = 0; i < list->num; i++) {
		if (!list->item[i])
			continue;
		
		result.item[i] = &result.buffer[result.writePoint];
		strcpy(result.item[i], list->item[i]);
		result.writePoint += strlen(result.item[i]) + 1;
	}
	
	list[0] = result;
	
	ItemList_Free(&vanilla);
	ItemList_Free(&modified);
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