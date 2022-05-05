#include "src/z64rom.h"
#include "src/Make.h"
#include <xm.h>

// TODO:
/*

   [ C - cfg patch overlap notify. Tell what cfg patch value gets ignored in the process ]

 */

const char* gToolName = PRNT_BLUE "z64rom " PRNT_GRAY "0.5.4"
#ifndef NDEBUG
	PRNT_DGRY " DEBUG BUILD"
#endif
;
char* sToolUsage = {
	EXT_INFO_TITLE("Usage:")
	EXT_INFO("Dump", 12, "Drag and drop a rom on z64rom")
	EXT_INFO("Build", 12, "Launch z64rom (after dump)")
	PRNT_NL
};
s32 gExtractAudio = true;
s32 gPrintInfo;
s32 gGenericNames;
s32 sDumpFlag;
s32 gInfoFlag;
s32 gLogOutput;
s32 gMakeForce;
const char* gMakeTarget;
extern u32 gThreading;

static void Main_Config(char** input, Rom* rom, s32 argc, char* argv[]) {
	const char* confRom = "z64project.cfg";
	MemFile* config = &rom->config;
	u32 parArg = 0;
	
	*config = MemFile_Initialize();
	
	if (input[0] == NULL) {
		if (ParseArgs(argv, "input", &parArg) || ParseArgs(argv, "i", &parArg)) {
			input[0] = argv[parArg];
		} else if (argc == 2) {
			if (StrEndCase(argv[1], ".z64")) {
				input[0] = argv[1];
				sDumpFlag = true;
				Sys_Delete(confRom);
			}
		}
	}
	
	if (!Sys_Stat(confRom)) {
		sDumpFlag = true;
		MemFile_Reset(config);
		MemFile_Malloc(config, MbToBin(2.5));
		
		MemFile_Printf(config, "# Project Settings\n");
		MemFile_Printf(config, "%-15s = \"%s\"\n", "z_baserom", String_GetFilename(input[0]));
		MemFile_Printf(config, "%-15s = \"%s\" # [oot_debug/oot_u10]\n", "z_rom_type", "__PLACEHOLDER__");
		
		MemFile_Printf(config, "\n# Mips64 Flags\n");
		
		MemFile_Printf(
			config,
			"%-15s = \"%s\"\n",
			"mips64_gcc_flags",
			"-c -Iinclude/z64hdr -Iinclude/z64hdr/include -Iinclude/ "
			"-Isrc/lib_user -G 0 -O1 -fno-reorder-blocks -std=gnu99 -march=vr4300 -mabi=32"
			" -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division -Wall"
			" -Wno-builtin-declaration-mismatch -Wno-unused-variable"
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
	} else {
		MemFile_Reset(config);
		MemFile_LoadFile_String(config, confRom);
		
		if (sDumpFlag == false) {
			input[0] = Config_GetString(config, "z_baserom");
			
			if (!Sys_Stat(Tmp_Printf("%s%s", Sys_AppDir(), input[0]))) {
				printf_error("Could not locate your baserom [%s]", Tmp_Printf("%s%s", Sys_AppDir(), input[0]));
			}
		}
	}
}

static void Main_RenameRooms(const char* from, const char* to) {
	ItemList list = ItemList_Initialize();
	u32 times = 0;
	
	printf_toolinfo(gToolName, "Room Extension Rename\n\n");
	
	if (!Sys_Stat("z64project.cfg"))
		printf_warning("No project found " PRNT_DGRY "[z64project.cfg]");
	
	ItemList_List(&list, "rom/scene/", -1, LIST_FILES);
	
	for (s32 i = 0; i < list.num; i++) {
		char* tmp;
		
		if (!StrEndCase(list.item[i], from))
			continue;
		
		tmp = Tmp_String(list.item[i]);
		String_Replace(tmp, from, to);
		
		if (Sys_Rename(list.item[i], tmp)) {
			printf_warning("Could not rename [%s] to \"%s\" format", list.item[i], to);
		}
		times++;
	}
	
	printf_info("%d rooms renamed to *%s.", times, to);
}

s32 Main(s32 argc, char* argv[]) {
	char* input = NULL;
	Rom* rom;
	u32 parArg = 0;
	u32 setupFlag = false;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Sys_SetWorkDir(Sys_AppDir());
	
	rom = Calloc(0, sizeof(struct Rom));
	
	if (Arg("zmap")) {
		Main_RenameRooms(".zroom", ".zmap");
		goto free;
	}
	
	if (Arg("zroom")) {
		Main_RenameRooms(".zmap", ".zroom");
		goto free;
	}
	
	if (Arg("B")) gMakeForce = true;
	
	if (Arg("M")) gMakeTarget = argv[parArg];
	
	if (Arg("single-thread"))
		gThreading = false;
	
	if (Arg("generic"))
		gGenericNames = true;
	
	if (Arg("no-wav"))
		gExtractAudio = false;
	
	if (Arg("info"))
		gPrintInfo = true;
	
	if (Arg("compress"))
		gCompressFlag = true;
	
	if (Arg("actor")) {
		u32 id = String_GetInt(argv[parArg]);
		
		if (Arg("i")) {
			input = argv[parArg];
			rom->type = Zelda_OoT_Debug;
			Rom_New(rom, input);
			Rom_Debug_ActorEntry(rom, id);
		}
		
		goto free;
	}
	
	if (Arg("dma")) {
		u32 id = String_GetInt(argv[parArg]);
		
		gInfoFlag = true;
		if (Arg("i")) {
			input = argv[parArg];
			rom->type = Zelda_OoT_Debug;
			Rom_New(rom, input);
			Rom_Debug_DmaEntry(rom, id);
		}
		
		goto free;
	}
	
	switch (setupFlag = Tools_Init()) {
		case -1:
			goto free;
			break;
			
		case 0:
			Main_Config(&input, rom, argc, argv);
			
			if (sDumpFlag == false) {
				if (Arg("update")) {
					printf_toolinfo(gToolName, "Updating z64hdr...\n\n");
					Tools_Update_Header();
					
					goto free;
				}
				
				if (Sys_Stat("z64project.cfg")) {
					if (!Arg("no-make")) {
						printf_toolinfo(gToolName, "\n");
						Make(rom);
					}
					if (Arg("make-only")) goto free;
				}
			}
			
			break;
			
		case 1:
#if 0
			printf("\n");
			printf_info("If you have a copy of " PRNT_REDD "Blender" PRNT_RSET ", drag n drop it here.");
			printf_info("If you're not planning to use custom objects,");
			printf_info("just answer " PRNT_BLUE "n" PRNT_RSET ".");
			
			MemFile mem =  MemFile_Initialize();
			
			MemFile_Malloc(&mem, 0x800);
			MemFile_Printf(&mem, "\n# Additional Tools\n");
			MemFile_Printf(&mem, "%-15s = %s", "blender_path\n", Terminal_GetStr());
			
			MemFile_SaveFile_String(&mem, "tools/ToolPaths.cfg");
			
			Terminal_ClearLines(2);
#endif
			printf("\n");
			for (s32 i = 0; i < argc; i++) {
				if (StrEnd(argv[i], ".z64") || StrEnd(argv[i], ".Z64")) {
					char* filename = String_GetFilename(argv[i]);
					
					if (!Sys_Stat(filename))
						Sys_Copy(argv[i], filename, false);
					input = filename;
					
					break;
				}
			}
			
			if (input == NULL) {
				ItemList list = ItemList_Initialize();
				
				ItemList_List(&list, "", 0, LIST_FILES);
				
				for (s32 i = 0; i < list.num; i++) {
					if (StrEndCase(list.item[i], ".z64") && !StrStr(list.item[i], "build")) {
						printf_info("Looks like you have a rom called " PRNT_REDD "%s " PRNT_RSET "in the same directory.", list.item[i]);
						
						printf_info("Want to use it as your baserom and dump it now? " PRNT_DGRY "[y/n]");
						
						if (Terminal_YesOrNo()) {
							input = strdup(list.item[i]);
							sDumpFlag = true;
						}
						
						Terminal_ClearLines(2);
						printf("\n");
						
						break;
					}
				}
				
				ItemList_Free(&list);
			}
			
			Main_Config(&input, rom, argc, argv);
			
			break;
	}
	
	if (input) {
		printf_toolinfo(gToolName, "\n");
		if (sDumpFlag) {
			if ((argc == 2 || setupFlag) && gExtractAudio) {
				printf_info("Extract " PRNT_REDD "wav audio samples" PRNT_RSET "?");
				printf_info("This is optional and will slow down dumping process. " PRNT_DGRY "[y/n]");
				if (!Terminal_YesOrNo()) gExtractAudio = false;
				Terminal_ClearLines(2);
				printf("\n");
			}
			
			Rom_New(rom, input);
			Rom_Dump(rom);
		} else {
			Rom_New(rom, input);
			Rom_Build(rom);
		}
		printf_info("Done!\n");
	}
	
	printf_toolinfo(gToolName, sToolUsage);
free:
	
	if (rom->config.dataSize)
		MemFile_SaveFile_String(&rom->config, "z64project.cfg");
	
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