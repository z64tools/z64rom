#include "src/z64rom.h"
#include "src/Make.h"
#include "src/Package.h"
#include <xm.h>

const char* gToolName = PRNT_BLUE "z64rom " PRNT_GRAY "0.8.1";
s32 gExtractAudio = true;
s32 gPrintInfo;
s32 gInfoFlag;
s32 gMakeForce;
const char* gMakeTarget;
u32 gCompressFlag = false;
s32 gBuildTarget = ROM_DEV;
char gRomName_Output[2][128] = {
	/* "build-release.z64", */
	/* "build-dev.z64", */
};
s32 gDumpFlag;
s32 gAudioOnly;
const char* sRomName[] = { "-release.z64", "-dev.z64" };

static void Main_RenameRooms(const char* from, const char* to) {
	ItemList list = ItemList_Initialize();
	u32 times = 0;
	
	printf_toolinfo(gToolName, "Room Extension Rename");
	
	if (!Sys_Stat("z64project.toml"))
		printf_warning("No project found " PRNT_DGRY "[z64project.toml]");
	
	ItemList_List(&list, "rom/scene/", -1, LIST_FILES);
	
	for (s32 i = 0; i < list.num; i++) {
		char* tmp;
		
		if (!StrEndCase(list.item[i], from))
			continue;
		
		tmp = HeapStrDup(list.item[i]);
		String_Replace(tmp, from, to);
		
		if (Sys_Rename(list.item[i], tmp)) {
			printf_warning("Could not rename [%s] to \"%s\" format", list.item[i], to);
		}
		times++;
	}
	
	printf_info("%d rooms renamed to *%s.", times, to);
}

static s32 Main_Arguments(Rom* rom, char* input, char* argv[]) {
	u32 parArg = 0;
	
	Log("Going through arguments...");
	
	if (Arg("zmap")) {
		Main_RenameRooms(".zroom", ".zmap");
		
		return 1;
	}
	
	if (Arg("zroom")) {
		Main_RenameRooms(".zmap", ".zroom");
		
		return 1;
	}
	
	if (Arg("force")) gMakeForce = true;
	
	if (Arg("target")) gMakeTarget = argv[parArg];
	
	if (Arg("no-threading"))
		gThreading = false;
	
	if (Arg("no-wav"))
		gExtractAudio = false;
	
	if (Arg("info"))
		gPrintInfo = true;
	
	if (Arg("no-beta")) {
		Rom_DeleteUnusedContent(Zelda_OoT_Debug);
		
		return 1;
	}
	
	if (Arg("reinstall")) {
		MemFile mem = MemFile_Initialize();
		
		MemFile_Malloc(&mem, 0x10);
		MemFile_SaveFile(&mem, "tools/.installing");
		MemFile_Free(&mem);
	}
	
	if (Arg("clean")) {
		ItemList list = ItemList_Initialize();
		
		ItemList_List(&list, "", -1, LIST_FILES);
		forlist(i, list) {
			if (StrEndCase(list.item[i], ".o") || StrEndCase(list.item[i], ".elf") || StrEndCase(list.item[i], "entry.ld")) {
				printf_warning("rm " PRNT_BLUE "%s", list.item[i]);
				Sys_Delete(list.item[i]);
			}
		}
		ItemList_Free(&list);
		
		ItemList_List(&list, "", -1, LIST_FOLDERS);
		forlist(i, list) {
			if (StrEndCase(list.item[i], ".entry/")) {
				printf_warning("rm " PRNT_YELW "%s", list.item[i]);
				Sys_Delete_Recursive(list.item[i]);
			}
		}
		ItemList_Free(&list);
		
		return 1;
	}
	
	return 0;
}

static void Main_Config(char** input, Rom* rom) {
	const char* projectConfig = "z64project.toml";
	MemFile* config = &rom->config;
	
	*config = MemFile_Initialize();
	
	if (Sys_Stat(projectConfig)) {
		char* projectRom;
		char* buildRom;
		
		MemFile_LoadFile_String(config, projectConfig);
		projectRom = Toml_GetStr(config, "z_baserom");
		buildRom = Toml_GetStr(config, "z_buildrom");
		
		if (strlen(buildRom) > 128 - strlen(sRomName[1]))
			printf_error("z_buildrom name is too long");
		
		sprintf(gRomName_Output[0], "%s%s", buildRom, sRomName[0]);
		sprintf(gRomName_Output[1], "%s%s", buildRom, sRomName[1]);
		
		if (gBuildTarget == ROM_RELEASE) {
			if (Sys_Stat(gRomName_Output[ROM_DEV]) > Sys_Stat(gRomName_Output[ROM_RELEASE])) {
				gMakeForce = true;
			}
		} else {
			if (Sys_Stat(gRomName_Output[ROM_RELEASE]) > Sys_Stat(gRomName_Output[ROM_DEV])) {
				gMakeForce = true;
			}
		}
		
		if (!Sys_Stat(projectRom))
			printf_error("Could not locate your baserom [%s]", projectRom);
		
		if (*input) {
			if (!Sys_Stat(*input))
				printf_error("File does not exist [%s]", *input);
			
			for (s32 i = 0; i < 2; i++) {
				if (StrEndCase(*input, gRomName_Output[i])) {
					gCompressFlag = true;
					*input = projectRom;
					
					return;
				}
			}
			
			printf_toolinfo(gToolName, "Redump");
			
			printf_warning("Dump rom [%s] ? " PRNT_DGRY "[y/n]", *input);
			if (Terminal_YesOrNo() == false) {
				*input = projectRom;
				
				printf_getchar("Press enter to exit.");
				exit(0);
			}
			Terminal_ClearLines(2);
			
			MemFile_Clear(config);
			Sys_Delete(projectConfig);
		} else {
			*input = projectRom;
			
			return;
		}
	}
	
	if (*input && !Sys_Stat(Filename(*input))) {
		printf_info("Copying provided rom to z64rom directory.");
		Sys_Copy(*input, Filename(*input), false);
		
		*input = strdup(Filename(*input));
	}
	
	gDumpFlag = true;
	
	Log("Writing [%s]", projectConfig);
	MemFile_Reset(config);
	MemFile_Malloc(config, MbToBin(2.5));
	
	MemFile_Printf(config, "# Project Settings\n");
	if (*input)
		MemFile_Printf(config, "%-15s = \"%s\"\n", "z_baserom", Filename(*input));
	else
		MemFile_Printf(config, "%-15s = \"%s\"\n", "z_baserom", "__ROM_NAME__");
	
	MemFile_Printf(config, "%-15s = \"%s\"\n", "z_buildrom", "build");
	MemFile_Printf(config, "%-15s = \"%s\" # [oot_debug/oot_u10]\n", "z_rom_type", "__PLACEHOLDER__");
	
	MemFile_Printf(config, "\n# Mips64 Flags\n");
	
	MemFile_Printf(
		config,
		"%-15s = \"%s\"\n",
		"mips64_gcc_flags",
		"-c -Iinclude/z64hdr -Iinclude/z64hdr/include "
		"-Isrc/lib_user -G 0 -O1 -fno-reorder-blocks -fno-common -std=gnu99 -march=vr4300 -mabi=32"
		" -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division -Wall"
		" -Wno-builtin-declaration-mismatch"
	);
	MemFile_Printf(
		config,
		"%-15s = \"%s\"\n",
		"mips64_gcc_flags_code",
		"-mno-gpopt -fomit-frame-pointer"
	);
	MemFile_Printf(
		config,
		"%-15s = \"%s\"\n",
		"mips64_ld_flags",
		"-Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T z64hdr.ld -T objects.ld -T z_lib_user.ld --emit-relocs"
	);
	MemFile_Printf(
		config,
		"%-15s = \"%s\"\n",
		"ulib_ld_flags",
		"-Lrom/lib_user -Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ -T ulib_linker.ld -T objects.ld --emit-relocs"
	);
	
	return;
}

s32 Main(s32 argc, char* argv[]) {
	char* input = NULL;
	Rom* rom = NULL;
	u32 parArg = 0;
	u32 romCount = 0;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Sys_SetWorkDir(Sys_AppDir());
	
	Calloc(rom, sizeof(struct Rom));
	
	if (Arg("audio-only")) {
		gAudioOnly = true;
		
		if (Arg("dump")) {
			Log("Dump Rom [%s]", argv[parArg]);
			Rom_New(rom, argv[parArg]);
			AudioOnly_Dump(rom);
		} else if (Arg("build")) {
			rom->mem.sampleTbl = MemFile_Initialize();
			rom->mem.fontTbl = MemFile_Initialize();
			rom->mem.seqTbl = MemFile_Initialize();
			rom->mem.seqFontTbl = MemFile_Initialize();
			MemFile_Malloc(&rom->mem.sampleTbl, MbToBin(0.1));
			MemFile_Malloc(&rom->mem.fontTbl, MbToBin(0.1));
			MemFile_Malloc(&rom->mem.seqTbl, MbToBin(0.1));
			MemFile_Malloc(&rom->mem.seqFontTbl, MbToBin(0.1));
			
			gMakeTarget = "sound";
			
			Make_Sound();
			Make_Sequence();
			AudioOnly_Build(rom);
		}
		
		goto free;
	}
	
	for (s32 i = 1; i < argc; i++) {
		if (StrEndCase(argv[i], ".z64")) {
			if (romCount > 0)
				printf_error("Too many roms provided as arguments!");
			input = argv[i];
			romCount++;
			Log("Rom [%s]", input);
			
			if (StrStr(argv[i], sRomName[ROM_RELEASE]))
				gBuildTarget = ROM_RELEASE;
		}
		
		if (StrStr(argv[i], "z64project.toml")) {
			printf_toolinfo(gToolName, "Release Build");
			gBuildTarget = ROM_RELEASE;
		}
	}
	
	if (Arg("actor") && input) {
		u32 id = Value_Int(argv[parArg]);
		
		rom->type = Zelda_OoT_Debug;
		Rom_New(rom, input);
		Rom_Debug_ActorEntry(rom, id);
		
		exit(0);
	}
	
	if (Arg("dma") && input) {
		u32 id = Value_Int(argv[parArg]);
		
		rom->type = Zelda_OoT_Debug;
		Rom_New(rom, input);
		Rom_Debug_DmaEntry(rom, id);
		
		exit(0);
	}
	
	if (Arg("scene") && input) {
		u32 id = Value_Int(argv[parArg]);
		
		rom->type = Zelda_OoT_Debug;
		Rom_New(rom, input);
		Rom_Debug_SceneEntry(rom, id);
		
		exit(0);
	}
	
	if (Arg("yaz")) gCompressFlag = true;
	
	Main_Config(&input, rom);
	if (Main_Arguments(rom, input, argv))
		goto free;
	
	switch (Tools_Init()) {
		case -1: {
			Log("Tools init [-1]");
			goto free;
		}
		
		case 1: {
			Log("Tools init [1]");
			for (s32 i = 0; i < argc; i++) {
				if (StrEnd(argv[i], ".z64") || StrEnd(argv[i], ".Z64")) {
					char* filename = Filename(argv[i]);
					
					if (!Sys_Stat(filename))
						Sys_Copy(argv[i], filename, false);
					input = filename;
					
					break;
				}
			}
			
			break;
		}
		
		case 0: {
			MemFile umem = MemFile_Initialize();
			u32 doUpdate = 0;
			u32 time = Sys_Date(Sys_Time()).hour;
			
			if (!Sys_Stat("tools/.update-check")) {
				MemFile_Malloc(&umem, 512);
				MemFile_Printf(&umem, "%02d", time);
				MemFile_SaveFile_String(&umem, "tools/.update-check");
				doUpdate = true;
			} else {
				MemFile_LoadFile_String(&umem, "tools/.update-check");
				
				if (time != Value_Int(umem.str)) {
					doUpdate = true;
					
					MemFile_Seek(&umem, 0);
					MemFile_Printf(&umem, "%02d", time);
					MemFile_SaveFile_String(&umem, "tools/.update-check");
				}
			}
			
			MemFile_Free(&umem);
			
			if (doUpdate)
				Tools_CheckUpdates();
			break;
		}
	}
	
	if (input == NULL) {
		ItemList list = ItemList_Initialize();
		
		ItemList_List(&list, "", 0, LIST_FILES);
		
		for (s32 i = 0; i < list.num; i++) {
			if (StrEndCase(list.item[i], ".z64") && !StrStr(list.item[i], "build")) {
				printf_toolinfo(gToolName, "");
				printf_info("Looks like you have a rom called " PRNT_REDD "%s " PRNT_RSET "in the same directory.", list.item[i]);
				
				printf_info("Want to use it as your baserom and dump it now? " PRNT_DGRY "[y/n]");
				
				if (Terminal_YesOrNo()) {
					input = StrDup(list.item[i]);
					gDumpFlag = true;
					
					String_Replace(rom->config.str, "__ROM_NAME__", input);
				}
				
				printf("\n");
				
				break;
			}
		}
		
		ItemList_Free(&list);
	}
	
	if (input) {
		printf_toolinfo(gToolName, "");
		
		if (gDumpFlag) {
			s32 soundsDumped = false;
			
			if (Sys_Stat("rom/sound/sample/.vanilla/")) {
				ItemList list = ItemList_Initialize();
				
				ItemList_List(&list, "rom/sound/sample/.vanilla/", -1, LIST_FILES | LIST_RELATIVE);
				
				for (s32 i = 0, j = 0; i < list.num; i++) {
					if (StrEndCase(list.item[i], ".wav")) {
						j++;
						if (j > 400) {
							printf_info("WAV dump already exists.");
							printf_info("Want to redump WAV anyway? " PRNT_DGRY "[y/n]");
							if (Terminal_YesOrNo()) {
								gExtractAudio = true;
							} else {
								gExtractAudio = false;
							}
							Terminal_ClearLines(3);
							
							soundsDumped = true;
							
							break;
						}
					}
				}
				
				ItemList_Free(&list);
			}
			
			if (gExtractAudio && soundsDumped == false) {
				printf_info("Extract " PRNT_REDD "wav audio samples" PRNT_RSET "?");
				printf_info("This is required if you want to make changes to audio. " PRNT_DGRY "[y/n]");
				if (!Terminal_YesOrNo()) gExtractAudio = false;
				
				printf("\n");
			}
			
			Rom_New(rom, input);
			
			Rom_Dump(rom);
		} else {
			u32 pkgInstalled = 0;
			for (s32 i = 0; i < argc; i++) {
				if (StrStrCase(argv[i], ".zip")) {
					printf_toolinfo(gToolName, "Installing Package(s)");
					printf_info_align("Package", "" PRNT_BLUE "%s", argv[i]);
					Package_Load(argv[i]);
					pkgInstalled = true;
				}
			}
			
			if (pkgInstalled) {
				printf_info("All packages have been installed successfully!\n");
				goto free;
			}
			
			if (Arg("update")) {
				printf_toolinfo(gToolName, "Updating z64hdr...");
				Tools_Install_z64hdr(true);
				
				goto free;
			}
			
			if (Sys_Stat("z64project.toml")) {
				if (!Arg("no-make")) {
					printf_toolinfo(gToolName, "");
					Make(rom, true);
				}
				if (Arg("make-only")) goto free;
			}
			
			Rom_New(rom, input);
			Rom_Build(rom);
		}
		printf_info("Done!\n");
	}
	
	printf_toolinfo(gToolName, "Nothing provided, nothing happens!");
	
free:
	
	if (input && rom->config.dataSize)
		MemFile_SaveFile_String(&rom->config, "z64project.toml");
	
	if (Arg("log"))
		Log_Print();
	
#ifdef _WIN32
	if (!Arg("no-wait")) {
		printf_getchar("Press enter to exit.");
	}
#endif
	
	Rom_Free(rom);
	Free(rom);
	Log_Free();
	Sound_Xm_Stop();
	
	return 0;
}
