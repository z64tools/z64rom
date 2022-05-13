#include "src/z64rom.h"
#include "src/Make.h"
#include "src/Package.h"
#include <xm.h>

const char* gToolName = PRNT_BLUE "z64rom " PRNT_GRAY "0.5.8";
s32 gExtractAudio = true;
s32 gPrintInfo;
s32 gGenericNames;
s32 sDumpFlag;
s32 gInfoFlag;
s32 gMakeForce;
const char* gMakeTarget;
extern u32 gThreading;

#define ROM_RELEASE 0
#define ROM_DEV     1

s32 gBuildTarget = ROM_DEV;
const char* gRomName_Output[] = {
	"build-release.z64",
	"build-dev.z64",
};

s32 Main_IsSameFile(const char* new, const char* cur) {
	if (Sys_StatF(new, STAT_ACCS) != Sys_StatF(cur, STAT_ACCS))
		return false;
	if (Sys_StatF(new, STAT_MODF) != Sys_StatF(cur, STAT_MODF))
		return false;
	if (Sys_StatF(new, STAT_CREA) != Sys_StatF(cur, STAT_CREA))
		return false;
	
	const char* pathA = String_GetPath(String_Unquote(new));
	const char* pathB = Sys_AppDir();
	
	if (strlen(pathB) != strlen(pathA))
		return false;
	
	if (!StrMtch(pathA, pathB))
		return false;
	
	return true;
}

void Main_SuperCompression(const char* input) {
	s32 dropTo = 0;
	
	printf_toolinfo(gToolName, "The Most Overtuned Ultimate Compressor");
	printf_info("Attempting to compress [" PRNT_REDD "%s" PRNT_RSET "] to achieve negative file size.", input);
	printf_info("This might take a while...\n");
	
	for (s32 i = 0; i < 99; i++) {
		f32 sec;
		
		sec = RandF() + RandF() * 1.56;
		
		printf_progressFst("" PRNT_BLUE "Compressing", i, 100);
		sec = WrapF(sec, 0.656, 1.367);
		
		if (dropTo) {
			Sys_Sleep(sec * 0.001);
			i -= 2;
			if (i <= dropTo)
				dropTo = 0;
			continue;
		}
		
		if (i % 6 == 0) {
			if (25 + RandF() * 125 < i) {
				dropTo = i * 0.77;
				Sys_Sleep(1.0f);
				continue;
			}
		}
		
		Sys_Sleep(sec * 0.7);
	}
	
	printf_warning("Something went wrong...");
	printf_getchar("Press enter to exit.");
	
	exit(0);
}

void Main_CompressRom(const char* input) {
	char cmd[512];
	char romName[64];
	
	strcpy(romName, input);
	String_Insert(StrEnd(romName, ".z64"), "-yaz");
	
	printf_toolinfo(gToolName, "Compressing");
	
	Tools_Command(
		cmd,
		z64compress,
		"--in %s "
		"--out %s "
		"--mb 32 "
		"--codec yaz "
		"--cache rom/cache/ "
		"--dma \"0x12F70,1548\" "
		"--compress \"9-14,28-END\" "
		"--threads 8",
		
		input,
		romName
	);
	SysExe(cmd);
}

void Main_RenameRooms(const char* from, const char* to) {
	ItemList list = ItemList_Initialize();
	u32 times = 0;
	
	printf_toolinfo(gToolName, "Room Extension Rename");
	
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

s32 Main_Arguments(Rom* rom, char* input, char* argv[]) {
	u32 parArg = 0;
	
	if (Arg("zmap")) {
		Main_RenameRooms(".zroom", ".zmap");
		
		return 1;
	}
	
	if (Arg("zroom")) {
		Main_RenameRooms(".zmap", ".zroom");
		
		return 1;
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
	
	if (Arg("actor") && input) {
		u32 id = String_GetInt(argv[parArg]);
		
		rom->type = Zelda_OoT_Debug;
		Rom_New(rom, input);
		Rom_Debug_ActorEntry(rom, id);
		
		return 1;
	}
	
	if (Arg("dma") && input) {
		u32 id = String_GetInt(argv[parArg]);
		
		gInfoFlag = true;
		rom->type = Zelda_OoT_Debug;
		Rom_New(rom, input);
		Rom_Debug_DmaEntry(rom, id);
		
		return 1;
	}
	
	return 0;
}

void Main_Config(char** input, Rom* rom) {
	const char* projectConfig = "z64project.cfg";
	MemFile* config = &rom->config;
	
	*config = MemFile_Initialize();
	
	if (Sys_Stat(projectConfig)) {
		char* projectRom;
		
		MemFile_LoadFile_String(config, projectConfig);
		projectRom = Config_GetString(config, "z_baserom");
		
		if (!Sys_Stat(projectRom))
			printf_error("Could not locate your baserom [%s]", projectRom);
		
		if (*input) {
			if (!Sys_Stat(*input))
				printf_error("File does not exist [%s]", *input);
			
			for (s32 i = 0; i < 2; i++) {
				if (Main_IsSameFile(*input, gRomName_Output[i])) {
					Main_CompressRom(gRomName_Output[i]);
					
					Terminal_GetChar();
					exit(0);
				}
			}
			
			if (Main_IsSameFile(*input, "build-dev-yaz.z64"))
				Main_SuperCompression("build-dev-yaz.z64");
			
			if (Main_IsSameFile(*input, "build-release-yaz.z64"))
				Main_SuperCompression("build-release-yaz.z64");
			
			if (Main_IsSameFile(*input, projectRom))
				return;
			
			printf_toolinfo(gToolName, "");
			
			printf_warning("Dump rom [%s] ? " PRNT_DGRY "[y/n]", *input);
			if (Terminal_YesOrNo() == false) {
				*input = projectRom;
				
				printf_getchar("Press enter to exit.");
				exit(0);
			}
			
			MemFile_Clear(config);
			Sys_Delete(projectConfig);
		} else {
			*input = projectRom;
			
			return;
		}
	}
	
	if (*input && !Sys_Stat(String_GetFilename(*input))) {
		printf_info("Copying provided rom to z64rom directory.");
		Sys_Copy(*input, String_GetFilename(*input), false);
		
		*input = strdup(String_GetFilename(*input));
	}
	
	sDumpFlag = true;
	
	MemFile_Reset(config);
	MemFile_Malloc(config, MbToBin(2.5));
	
	MemFile_Printf(config, "# Project Settings\n");
	if (*input)
		MemFile_Printf(config, "%-15s = \"%s\"\n", "z_baserom", *input);
	else
		MemFile_Printf(config, "%-15s = \"%s\"\n", "z_baserom", "__ROM_NAME__");
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
	
	return;
}

void Main_z64project() {
	printf_toolinfo(gToolName, "Release Build");
	
	gBuildTarget = false;
	
	if (Sys_Stat(gRomName_Output[ROM_DEV]) > Sys_Stat(gRomName_Output[ROM_RELEASE])) {
		gMakeForce = true;
	}
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
	
	rom = Calloc(rom, sizeof(struct Rom));
	
	for (s32 i = 1; i < argc; i++) {
		if (StrEndCase(argv[i], ".z64")) {
			if (romCount > 0)
				printf_error("Too many roms provided as arguments!");
			input = argv[i];
			romCount++;
		}
		
		if (Main_IsSameFile(argv[i], "z64project.cfg"))
			Main_z64project();
	}
	
	Main_Config(&input, rom);
	if (Main_Arguments(rom, input, argv))
		goto free;
	
	switch (Tools_Init()) {
		case -1: {
			goto free;
		}
		
		case 1: {
			for (s32 i = 0; i < argc; i++) {
				if (StrEnd(argv[i], ".z64") || StrEnd(argv[i], ".Z64")) {
					char* filename = String_GetFilename(argv[i]);
					
					if (!Sys_Stat(filename))
						Sys_Copy(argv[i], filename, false);
					input = filename;
					
					break;
				}
			}
			
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
					input = strdup(list.item[i]);
					sDumpFlag = true;
					
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
		
		if (sDumpFlag) {
			if (gExtractAudio) {
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
				printf_info("All packages have been imported successfully!\n");
				goto free;
			}
			
			if (gBuildTarget == true) {
				if (Sys_Stat(gRomName_Output[ROM_RELEASE]) > Sys_Stat(gRomName_Output[ROM_DEV])) {
					gMakeForce = true;
				}
			}
			
			if (Arg("update")) {
				printf_toolinfo(gToolName, "Updating z64hdr...");
				Tools_Update_Header(false);
				
				goto free;
			}
			
			if (Sys_Stat("z64project.cfg")) {
				if (!Arg("no-make")) {
					printf_toolinfo(gToolName, "");
					Make(rom);
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
