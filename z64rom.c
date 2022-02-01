#include "lib/z64rom.h"

char* sToolName = PRNT_PRPL "z64rom " PRNT_GRAY "0.4.0.0"
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
s32 gLog;
s32 gGenericNames;
s32 sDumpFlag;

void z64rom_Config(char** input, Rom* rom, s32 argc, char* argv[]);
void z64rom_CheckTypes();

void sleep(u32);

s32 Main(s32 argc, char* argv[]) {
	char* input = NULL;
	Rom* rom = Lib_Calloc(0, sizeof(struct Rom));
	u32 parArg = 0;
	
	#ifdef _WIN32
		printf_WinFix();
	#endif
	printf_SetPrefix("");
	
	if (ParArg("--actor")) {
		u32 id = String_GetInt(argv[parArg]);
		
		if (ParArg("--i")) {
			input = argv[parArg];
			rom->type = Zelda_OoT_Debug;
			Rom_New(rom, input);
			Rom_Debug_ActorEntry(rom, id);
			Rom_Free(rom);
		}
		
		return 0;
	}
	
	if (ParArg("--dma")) {
		u32 id = String_GetInt(argv[parArg]);
		
		if (ParArg("--i")) {
			input = argv[parArg];
			rom->type = Zelda_OoT_Debug;
			Rom_New(rom, input);
			Rom_Debug_DmaEntry(rom, id);
			Rom_Free(rom);
		}
		
		return 0;
	}
	
	if (ParArg("--generic"))
		gGenericNames = true;
	
	if (ParArg("--L"))
		gLog = true;
	
	if (ParArg("--D"))
		printf_SetSuppressLevel(PSL_DEBUG);
	
	z64rom_CheckTypes();
	z64rom_Config(&input, rom, argc, argv);
	
	if (input) {
		if (sDumpFlag) {
			printf_toolinfo(sToolName, "\n");
			Rom_New(rom, input);
			Rom_Dump(rom);
			Rom_Free(rom);
			printf_info("Done!");
			
			#ifdef _WIN32
				sleep(1);
			#endif
			
			return 0;
		} else {
			printf_toolinfo(sToolName, "\n");
			Rom_New(rom, input);
			Rom_Build(rom);
			Rom_Free(rom);
			printf_info("Done!");
			
			#ifdef _WIN32
				sleep(1);
			#endif
			
			return 0;
		}
	}
	
	printf_toolinfo(sToolName, sToolUsage);
	
	#ifdef _WIN32
		if (argc == 1) {
			printf_info("Press enter to exit.");
			getchar();
		}
	#endif
	
	return 0;
}

void z64rom_Config(char** input, Rom* rom, s32 argc, char* argv[]) {
	char* confAu = tprintf("%s%s", CurWorkDir(), "tools/z64audio.cfg");
	char* confRom = tprintf("%s%s", CurWorkDir(), "z64project.cfg");
	MemFile* config = &rom->config;
	u32 parArg = 0;
	
	*config = MemFile_Initialize();
	
	if (!Stat(confAu)) {
		MemFile config = MemFile_Initialize();
		#ifndef _WIN32
			if (system("./tools/z64audio --GenCfg --S")) {
			}
		#else
			if (system("tools\\z64audio.exe --GenCfg --S")) {
			}
		#endif
		
		if (MemFile_LoadFile_String(&config, confAu))
			printf_error("Could not locate [tools/config.cfg]");
		
		String_Replace(config.data, "zaudio_z64rom_mode = false", "zaudio_z64rom_mode = true");
		config.dataSize = strlen(config.data);
		MemFile_SaveFile_String(&config, "tools/z64audio.cfg");
		MemFile_Free(&config);
	}
	
	if (Lib_ParseArguments(argv, "--i", &parArg)) {
		input[0] = String_GetSpacedArg(argv, parArg);
		sDumpFlag = true;
	} else {
		for (s32 i = 0; i < argc; i++) {
			if (String_MemMemCase(argv[i], ".z64")) {
				input[0] = argv[i];
				sDumpFlag = true;
				break;
			}
		}
	}
	
	if (sDumpFlag) {
		MemFile_Malloc(config, MbToBin(0.1));
		
		Config_WriteTitle_Str("Project Settings");
		Config_WriteVar_Str("z_baserom", String_GetFilename(input[0]));
		
		MemFile_SaveFile_String(config, confRom);
	} else if (Stat(confRom)) {
		MemFile_LoadFile_String(config, confRom);
		
		input[0] = Config_GetString(config, "z_baserom");
		
		if (!Stat(tprintf("%s%s", CurWorkDir(), input[0]))) {
			printf_error("Could not locate your baserom [%s]", tprintf("%s%s", CurWorkDir(), input[0]));
		}
	}
}

void z64rom_CheckTypes() {
	u32 error = 0;
	
	#define SizeTester(type, expectedSize) if (sizeof(type) != expectedSize) { \
			printf_error_align("sizeof(" # type ")", "%d > %d", sizeof(type), expectedSize); \
			error++; \
	}
	SizeTester(enum SampleMedium, 1);
	SizeTester(enum SeqPlayer, 1);
	SizeTester(struct AudioEntry, 16);
	SizeTester(struct Instrument, 32);
	SizeTester(struct Adsr, 4);
	SizeTester(struct Sound, 8);
	SizeTester(struct AudioEntryHead, 16);
	
	if (error) {
		exit(EXIT_FAILURE);
	}
}