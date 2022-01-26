#include "z64rom.h"

void Rom_ItemList(ItemList* list, bool isPath, bool isNum, bool numericalSort) {
	ItemList listVan = { 0 };
	ItemList listMod = { 0 };
	u32 posVan = 0;
	u32 posMod = 0;
	
	Dir_Enter(".vanilla/"); {
		Dir_ItemList(&listVan, isPath);
	} Dir_Leave();
	Dir_ItemList(&listMod, isPath);
	
	if (numericalSort) {
		ItemList_NumericalSort(&listVan);
		ItemList_NumericalSort(&listMod);
	}
	
	*list = (ItemList) { 0 };
	
	list->buffer = Graph_Alloc((listMod.writePoint + listVan.writePoint) * 2);
	list->item = Graph_Alloc(sizeof(u8*) * (listMod.num + listVan.num));
	
	if (isNum) {
		for (s32 i = 0;; i++) {
			if (posMod >= listMod.num && posVan >= listVan.num) {
				break;
			}
			
			if (posMod < listMod.num && String_GetInt(listMod.item[posMod]) == i) {
				list->item[list->num] = &list->buffer[list->writePoint];
				strcpy(list->item[list->num], listMod.item[posMod]);
				list->writePoint += strlen(listMod.item[posMod]) + 1;
				
				list->num++;
				posMod++;
				posVan++;
			} else if (posVan < listVan.num && String_GetInt(listVan.item[posVan]) == i) {
				char* item = tprintf(".vanilla/%s", listVan.item[posVan]);
				
				list->item[list->num] = &list->buffer[list->writePoint];
				strcpy(list->item[list->num], item);
				list->writePoint += strlen(item) + 1;
				
				list->num++;
				posVan++;
			}
		}
	} else {
		while (posMod < listMod.num) {
			list->item[list->num] = &list->buffer[list->writePoint];
			strcpy(list->item[list->num], listMod.item[posMod]);
			list->writePoint += strlen(listMod.item[posMod]) + 1;
			
			list->num++;
			posMod++;
		}
		
		while (posVan < listVan.num) {
			u32 cont = 0;
			for (s32 i = 0; i < list->num; i++) {
				if (!strcmp(listVan.item[posVan], list->item[i])) {
					cont = 1;
					posVan++;
					break;
				}
			}
			
			if (cont) continue;
			
			char* item = tprintf(".vanilla/%s", listVan.item[posVan]);
			
			list->item[list->num] = &list->buffer[list->writePoint];
			strcpy(list->item[list->num], item);
			list->writePoint += strlen(item) + 1;
			
			list->num++;
			posVan++;
		}
	}
	
	#ifndef NDEBUG
		if (gPrintfSuppress == PSL_DEBUG) {
			printf_debugExt("RomList");
			for (s32 i = 0; i < list->num; i++) {
				if (i == 0) {
					printf_debugExt("%s", list->item[i]);
				} else {
					printf_debug("%s", list->item[i]);
				}
			}
			printf_warning("Looks OK? [Y/N]");
			if (!printf_get_answer()) {
				exit(1);
			}
		}
	#endif
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

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

static void Rom_Config_Actor(MemFile* config, ActorEntry* actorOvl, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Hex("vram_addr", ReadBE(actorOvl->vramStart));
	Config_WriteVar_Hex("init_vars", ReadBE(actorOvl->initInfo));
	Config_WriteVar_Int("alloc_type", ReadBE(actorOvl->allocType));
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

static void Rom_Config_Player(Rom* rom, MemFile* config, KaleidoEntry* player, const char* name, char* out) {
	u16* dataHi;
	u16* dataLo;
	u32 init;
	u32 dest;
	u32 updt;
	u32 draw;
	
	dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.init.hi);
	dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.init.lo);
	init = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
	
	dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.dest.hi);
	dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.dest.lo);
	dest = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
	
	dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.updt.hi);
	dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.updt.lo);
	updt = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
	
	dataHi = SegmentedToVirtual(0x0, rom->offset.table.player.draw.hi);
	dataLo = SegmentedToVirtual(0x0, rom->offset.table.player.draw.lo);
	draw = ReadBE(dataHi[1]) << 16 | ReadBE(dataLo[1]);
	
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Hex("vram_addr", ReadBE(player->vramStart));
	Config_WriteVar_Hex("init", init);
	Config_WriteVar_Hex("dest", dest);
	Config_WriteVar_Hex("updt", updt);
	Config_WriteVar_Hex("draw", draw);
	MemFile_SaveFile_String(config, out);
}

static void Rom_Config_Scene(MemFile* config, SceneEntry* sceneEntry, const char* name, char* out) {
	MemFile_Reset(config);
	Config_WriteTitle_Str(name);
	Config_WriteVar_Int("unk_a", ReadBE(sceneEntry->unk_10));
	Config_WriteVar_Int("unk_b", ReadBE(sceneEntry->unk_12));
	Config_WriteVar_Int("shader", ReadBE(sceneEntry->config));
	MemFile_SaveFile_String(config, out);
}

/* / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / * / */

static void Rom_Build_Patch(Rom* rom, MemFile* dataFile, MemFile* config) {
	ItemList list;
	
	Dir_ItemList_Keyword(&list, ".bin");
	
	for (s32 i = 0; i < list.num; i++) {
		printf_progress("Applying Patches", i + 1, list.num);
		u32 injectAddr;
		char* addrPoint = String_MemMem(list.item[i], "0x");
		
		MemFile_Reset(dataFile);
		MemFile_LoadFile(dataFile, list.item[i]);
		
		injectAddr = String_GetInt(addrPoint);
		
		MemFile_Seek(&rom->file, injectAddr);
		MemFile_Append(&rom->file, dataFile);
	}
}

void Rom_Dump(Rom* rom) {
	MemFile dataFile = MemFile_Initialize();
	MemFile config = MemFile_Initialize();
	RomFile rf;
	
	Dir_SetParam(DIR__MAKE_ON_ENTER);
	MemFile_Malloc(&dataFile, 0x460000); // Slightly larger than audiotable
	MemFile_Malloc(&config, 0x25000);
	
	printf_info_align("Dump Rom", PRNT_PRPL "%s", rom->file.info.name);
	Dir_Enter("rom/");
	#if 1
		Dir_Enter("actor/"); {
			for (s32 i = 0; i < rom->table.num.actor; i++) {
				rf = Dma_RomFile_Actor(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Actor", i + 1, rom->table.num.actor);
				Dir_Enter("0x%04X-%s/", i, gActorName[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File("actor.zovl")))
						Rom_Config_Actor(&config, &rom->table.actor[i], gActorName[i], Dir_File("config.cfg"));
				} Dir_Leave();
			}
		} Dir_Leave();
		
		Dir_Enter("object/"); {
			for (s32 i = 0; i < rom->table.num.obj; i++) {
				rf = Dma_RomFile_Object(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Object", i + 1, rom->table.num.obj);
				Dir_Enter("0x%04X-%s/", i, gObjectName[i]); {
					Rom_Extract(&dataFile, rf, Dir_File("object.zobj"));
				} Dir_Leave();
			}
		} Dir_Leave();
		
		Dir_Enter("system/"); {
			for (s32 i = 0; i < rom->table.num.state; i++) {
				rf = Dma_RomFile_GameState(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("System", i + 1, rom->table.num.state);
				Dir_Enter("GameState_%s/", gStateName[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File("state.zovl")))
						Rom_Config_GameState(&config, &rom->table.state[i], gStateName[i], Dir_File("config.cfg"));
				} Dir_Leave();
			}
			
			printf_info("Player");
			Dir_Enter("Player/"); {
				rf.size = ReadBE(rom->table.kaleido[1].vromEnd) - ReadBE(rom->table.kaleido[1].vromStart);
				rf.data = SegmentedToVirtual(0x0, ReadBE(rom->table.kaleido[1].vromStart));
				
				Rom_Extract(&dataFile, rf, Dir_File("EnPlayer.zovl"));
				Rom_Config_Player(rom, &config, &rom->table.kaleido[1], "Player", Dir_File("config.cfg"));
			} Dir_Leave();
		} Dir_Leave();
		
		Dir_Enter("scene/"); {
			for (s32 i = 0; i < rom->table.num.scene; i++) {
				rf = Dma_RomFile_Scene(rom, i);
				
				if (rf.size == 0)
					continue;
				
				printf_progress("Scene", i + 1, rom->table.num.scene);
				Dir_Enter("0x%02X-%s/", i, gSceneName[i]); {
					if (Rom_Extract(&dataFile, rf, Dir_File("scene.zscene"))) {
						u32* seg;
						u32 roomNum;
						u32 roomListSeg;
						u32* vromSeg;
						
						Rom_Config_Scene(&config, &rom->table.scene[i], gSceneName[i], Dir_File("config.cfg"));
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
								printf_debugExt_align("Room Extract", "Scene Segment %X", VirtualToSegmented(0x2, vromSeg));
								Rom_Extract(
									&dataFile,
									Rom_GetRomFile(rom, vromSeg[0], vromSeg[1]),
									out
								);
							}
						}
					}
				} Dir_Leave();
			}
		} Dir_Leave();
		SetSegment(0x2, NULL);
	#endif
	
	Dir_Enter("sound/"); {
		Rom_Dump_SoundFont(rom, &dataFile, &config);
		Rom_Dump_Sequences(rom, &dataFile, &config);
		Rom_Dump_Samples(rom, &dataFile, &config);
	} Dir_Leave();
	
	Dir_Leave();
	
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
	
	MemFile_Seek(&rom->file, 0x35D0000);
	
	Dir_Enter("rom/"); {
		Dir_Enter("sound/"); {
			Dir_Enter("sample/"); {
				Rom_Build_SampleTable(rom, &dataFile, &config);
			} Dir_Leave();
			
			Dir_Enter("soundfont/"); {
				Rom_Build_SoundFont(rom, &dataFile, &config);
			} Dir_Leave();
			
			Dir_Enter("sequence/"); {
				Rom_Build_Sequence(rom, &dataFile, &config);
			} Dir_Leave();
			Log("Segment_Sample %08X", rom->offset.segment.smplRom);
			Log("Segment_SoundF %08X", rom->offset.segment.fontRom);
			Log("Segment_Sequen %08X", rom->offset.segment.seqRom);
		} Dir_Leave();
	} Dir_Leave();
	
	Dir_SetParam(DIR__MAKE_ON_ENTER);
	Dir_Enter("patches/"); {
		Rom_Build_Patch(rom, &dataFile, &config);
	} Dir_Leave();
	Dir_UnsetParam(DIR__MAKE_ON_ENTER);
	
	Rom_Build_SetAudioSegment(rom);
	
	fix_crc(rom->file.data);
	MemFile_SaveFile(&rom->file, "build.z64");
	
	if (gLog)
		LogPrint();
}

void Rom_New(Rom* rom, char* romName) {
	u32* hdr;
	u16* addr;
	
	rom->file = MemFile_Initialize();
	MemFile_Params(&rom->file, MEM_FILENAME, true, MEM_END);
	if (MemFile_LoadFile(&rom->file, romName)) {
		printf_error_align("Load Rom", "%s", romName);
	}
	
	if (rom->file.dataSize < MbToBin(64)) {
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
		char* confRom = tprintf("%s%s", CurWorkDir(), "rom_config.cfg");
		MemFile conf = MemFile_Initialize();
		MemFile* config = &conf;
		
		MemFile_Malloc(config, 0x1000);
		MemFile_LoadFile_String(&conf, confRom);
		conf.seekPoint = strlen(conf.data);
		
		if (!Config_Get(config, "z_rom_type")) {
			if (hdr[0] != 0) {
				rom->type = Zelda_OoT_Debug;
				Config_WriteVar_Str("z_rom_type", "oot_debug # [oot_debug/oot_u10]");
			} else {
				rom->type = Zelda_OoT_1_0;
				Config_WriteVar_Str("z_rom_type", "oot_u10 # [oot_debug/oot_u10]");
			}
			MemFile_SaveFile_String(config, confRom);
		} else {
			char* romType = Config_GetString(config, "z_rom_type");
			
			if (!strcmp(romType, "oot_debug")) {
				rom->type = Zelda_OoT_Debug;
			} else if (!strcmp(romType, "oot_u10")) {
				rom->type = Zelda_OoT_1_0;
			} else {
				rom->type = NoRom;
			}
		}
		MemFile_Free(config);
	}
	
	switch (rom->type) {
		case Zelda_OoT_Debug:
			rom->offset.table.dmaTable = 0x012F70;
			rom->offset.table.objTable = 0xB9E6C8;
			rom->offset.table.actorTable = 0xB8D440;
			rom->offset.table.stateTable = 0xB969D0;
			rom->offset.table.sceneTable = 0xBA0BB0;
			rom->offset.table.kaleidoTable = 0xBA4340;
			
			rom->offset.table.seqFontTbl = 0xBCC4E0;
			rom->offset.table.seqTable = 0xBCC6A0;
			rom->offset.table.fontTable = 0xBCC270;
			rom->offset.table.sampleTable = 0xBCCD90;
			
			addr = SegmentedToVirtual(0x0, 0xB5A4AE);
			rom->offset.segment.seqRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
			rom->offset.segment.seqRom |= ReadBE(addr[2]);
			// rom->offset.segment.seqRom = 0x44DF0;
			printf_debug_align("SequenceRom", "%08X", rom->offset.segment.seqRom);
			
			addr = SegmentedToVirtual(0x0, 0xB5A4C2);
			rom->offset.segment.fontRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
			rom->offset.segment.fontRom |= ReadBE(addr[2]);
			// rom->offset.segment.fontRom = 0x19030;
			printf_debug_align("FontRom", "%08X", rom->offset.segment.fontRom);
			
			addr = SegmentedToVirtual(0x0, 0xB5A4D6);
			rom->offset.segment.smplRom = (ReadBE(addr[0]) - (ReadBE(addr[2]) > 0x7FFF)) << 16;
			rom->offset.segment.smplRom |= ReadBE(addr[2]);
			// rom->offset.segment.smplRom = 0x94870;
			printf_debug_align("SampleRom", "%08X", rom->offset.segment.smplRom);
			
			rom->table.num.dma = 1548;
			rom->table.num.obj = 402;
			rom->table.num.actor = 471;
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
			break;
		case Zelda_OoT_1_0:
			rom->offset.table.dmaTable = 0x00007430;
			rom->offset.table.objTable = 0x00B6EF58;
			rom->offset.table.actorTable = 0x00B5E490;
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
			
			rom->table.num.dma = 1526;
			rom->table.num.obj = 402;
			rom->table.num.actor = 471;
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
	rom->table.state = SegmentedToVirtual(0x0, rom->offset.table.stateTable);
	rom->table.scene = SegmentedToVirtual(0x0, rom->offset.table.sceneTable);
	rom->table.kaleido = SegmentedToVirtual(0x0, rom->offset.table.kaleidoTable);
	
	#ifdef NDEBUG // rezimodnar ksaM sarojaM esuaceb tsuj tseuQ drihT gnipmud diovA
		if (rom->offset.segment.seqRom == 0x03F00000 &&
			rom->offset.segment.fontRom == 0x03E00000 &&
			rom->offset.segment.smplRom == 0x00094870 &&
			rom->type == Zelda_OoT_Debug) {
			u32* checkVal = SegmentedToVirtual(0x0, 0xBCC920);
			
			if (ReadBE(checkVal[0]) == 0x52059 && ReadBE(checkVal[1]) == 0x37F7) {
				SwapBE(rom->offset.table.seqTable);
			}
		}
	#endif
}

void Rom_Free(Rom* rom) {
	MemFile_Free(&rom->file);
	memset(rom, 0, sizeof(struct Rom));
}
