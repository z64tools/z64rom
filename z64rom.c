#include "src/z64rom.h"
#include "src/Make.h"
#include <xm.h>

const char* gToolName = PRNT_BLUE "z64rom " PRNT_GRAY "0.5.0"
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
extern u32 gThreading;

static void Main_Config(char** input, Rom* rom, s32 argc, char* argv[]) {
	const char* confRom = "z64project.cfg";
	MemFile* config = &rom->config;
	u32 parArg = 0;
	
	*config = MemFile_Initialize();
	
	if (ParseArgs(argv, "input", &parArg) || ParseArgs(argv, "i", &parArg)) {
		input[0] = argv[parArg];
		sDumpFlag = true;
	} else if (argc == 2) {
		input[0] = argv[1];
		
		if (StrStrCase(input[0], ".z64")) sDumpFlag = true;
		else input[0] = NULL;
	}
	
	if (sDumpFlag) {
		MemFile_Reset(config);
		MemFile_Malloc(config, MbToBin(0.1));
		
		MemFile_Printf(config, "# Project Settings\n");
		MemFile_Printf(config, "%-15s = \"%s\"\n", "z_baserom", String_GetFilename(input[0]));
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
	} else if (Sys_Stat(confRom)) {
		MemFile_Reset(config);
		MemFile_LoadFile_String(config, confRom);
		
		input[0] = Config_GetString(config, "z_baserom");
		
		if (!Sys_Stat(Tmp_Printf("%s%s", Sys_AppDir(), input[0]))) {
			printf_error("Could not locate your baserom [%s]", Tmp_Printf("%s%s", Sys_AppDir(), input[0]));
		}
	}
}

s32 Main(s32 argc, char* argv[]) {
	char* input = NULL;
	Rom* rom;
	u32 parArg = 0;
	u32 setupFlag = false;
	
	rom = Calloc(0, sizeof(struct Rom));
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Sys_SetWorkDir(Sys_AppDir());
	
	if (XARG("single-thread")) gThreading = false;
	
	switch (setupFlag = Tools_Init()) {
		case -1:
			goto free;
			break;
			
		case 0:
			Main_Config(&input, rom, argc, argv);
			
			if (XARG("update")) {
				printf_toolinfo(gToolName, "Updating z64hdr...\n\n");
				Tools_Update_Header();
				
				goto free;
			}
			
			if (Sys_Stat("z64project.cfg")) {
				if (!XARG("no-make")) {
					printf_toolinfo(gToolName, "\n");
					Make(rom);
				}
				if (XARG("make-only")) goto free;
			}
			
			if (XARG("yaz")) {
				u8* tmpBuffer;
				char* newFileName = Tmp_Alloc(512);
				MemFile file = MemFile_Initialize();
				
				input = argv[parArg];
				String_SwapExtension(newFileName, input, ".yaz");
				printf_info("Compressing [%s] to [%s]", input, newFileName);
				
				if (MemFile_LoadFile(&file, input)) printf_error("Could not load [%s]", input);
				tmpBuffer = Tmp_Alloc(file.dataSize);
				file.dataSize = Yaz_Encode(tmpBuffer, file.data, file.dataSize);
				file.data = tmpBuffer;
				
				if (MemFile_SaveFile(&file, newFileName)) printf_error("Could not save [%s]", newFileName);
				
				goto free;
			}
			
			if (XARG("base") || XARG("mod")) {
				MemFile memBase = MemFile_Initialize();
				MemFile memMod = MemFile_Initialize();
				MemFile patch = MemFile_Initialize();
				char* base = NULL;
				char* mod = NULL;
				s32 cont = 0;
				s32 first = 0;
				
				if (XARG("base")) base = argv[parArg];
				if (XARG("mod")) mod = argv[parArg];
				
				if (mod == NULL || base == NULL)
					printf_error("Provide both [--base] and [--mod]");
				
				MemFile_Malloc(&patch, MbToBin(32.0));
				MemFile_LoadFile(&memBase, base);
				MemFile_LoadFile(&memMod, mod);
				
				for (s32 i = 0x40; i < (u32)fmax(memBase.dataSize, memMod.dataSize); i++) {
					if (memBase.cast.u8[i] == memMod.cast.u8[i]) {
						cont = 0;
						continue;
					}
					
					if (cont == 0) {
						if (first)
							MemFile_Printf(&patch, "\n");
						MemFile_Printf(&patch, "0x%08X = 0x", i);
						first = 1;
					}
					
					if (MemFile_Printf(&patch, "%02X", memMod.cast.u8[i])) {
						printf_error("Size exceeded %.0f MB. Exiting...", BinToMb(patch.memSize));
					}
					
					cont = 1;
				}
				
				if (patch.dataSize > 0) {
					// printf("%s\n", (char*)patch.data);
					MemFile_SaveFile_String(&patch, "PatchOutput.cfg");
				}
				MemFile_Free(&patch);
				MemFile_Free(&memBase);
				MemFile_Free(&memMod);
				
				goto free;
			}
			
			if (XARG("actor")) {
				u32 id = String_GetInt(argv[parArg]);
				
				if (XARG("i")) {
					input = argv[parArg];
					rom->type = Zelda_OoT_Debug;
					Rom_New(rom, input);
					Rom_Debug_ActorEntry(rom, id);
				}
				
				goto free;
			}
			
			if (XARG("dma")) {
				u32 id = String_GetInt(argv[parArg]);
				
				gInfoFlag = true;
				if (XARG("i")) {
					input = argv[parArg];
					rom->type = Zelda_OoT_Debug;
					Rom_New(rom, input);
					Rom_Debug_DmaEntry(rom, id);
				}
				
				goto free;
			}
			
			if (XARG("generic"))
				gGenericNames = true;
			
			if (XARG("no-wav"))
				gExtractAudio = false;
			
			if (XARG("info"))
				gPrintInfo = true;
			
			if (XARG("debug"))
				printf_SetSuppressLevel(PSL_DEBUG);
			
			if (XARG("compress"))
				gCompressFlag = true;
			
			break;
			
		case 1:
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
				
				ItemList_List(&list, "", 0);
				
				for (s32 i = 0; i < list.num; i++) {
					if ((StrEnd(list.item[i], ".z64") || StrEnd(list.item[i], ".Z64")) && !StrStr(list.item[i], "build")) {
						printf("\n");
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
			Rom_Free(rom);
			printf_info("Done!\n");
			
			goto free;
		} else {
			Rom_New(rom, input);
			Rom_Build(rom);
			Rom_Free(rom);
			printf_info("Done!\n");
			
			goto free;
		}
	}
	
	printf_toolinfo(gToolName, sToolUsage);
	
free:
#ifdef _WIN32
	
	if (!XARG("no-wait")) {
		printf_getchar("Press enter to exit.");
	}
#endif
	
	Free(rom);
	Log_Free();
	Sound_Xm_Stop();
	
	return 0;
}