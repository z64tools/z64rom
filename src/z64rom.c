#include "z64rom.h"
#include "Make.h"
#include "Package.h"
#include <xm.h>

const char* gToolName = PRNT_BLUE "z64rom " PRNT_GRAY "1.0.4";

s32 gDumpRom = -1;
s32 gDumpAudio = -1;
s32 gAutoInstall = -1;
const char* gFile_z64hdr;
const char* gFile_mips64;
const char* gWorkDir;

u32 gCompressFlag = false;
s32 gPrintInfo;
s32 gMakeForce;
const char* gMakeTarget;
u32 gThreading = true;
u32 gThreadNum = 64;
s32 gDumpFlag;

s32 gAudioOnly;
s32 gBuildTarget = ROM_DEV;
char gBuildrom[2][128] = {
	/* "build-release.z64", */
	/* "build-dev.z64", */
};

char* gVanilla = ".vanilla";
const char* gProjectConfig = "z64project.cfg";
const char* gBaserom = NULL;

const char* sRomType[] = { "-release.z64", "-dev.z64" };

static void MakeClean(void) {
	ItemList list = ItemList_Initialize();
	
	ItemList_List(&list, "rom/", -1, LIST_FILES);
	forlist(i, list) {
		if (StrEnd(list.item[i], ".o") || StrEnd(list.item[i], ".elf")) {
			if (Sys_Stat(Path(list.item[i])) && !StrStr(list.item[i], ".entry")) {
				printf_warning("rm " PRNT_BLUE "%s", Path(list.item[i]));
				Sys_Delete_Recursive(Path(list.item[i]));
			}
		}
	}
	ItemList_Free(&list);
	
	ItemList_List(&list, "rom/", -1, LIST_FOLDERS);
	forlist(i, list) {
		if (StrEndCase(list.item[i], ".entry/")) {
			printf_warning("rm " PRNT_YELW "%s", list.item[i]);
			Sys_Delete_Recursive(list.item[i]);
		}
	}
	ItemList_Free(&list);
}

static void PrintHelp(void) {
	printf_toolinfo(gToolName, "Help");
	
	printf("z64rom dev is lazy...\n" PRNT_BLUE "https://github.com/z64tools/z64rom/wiki\n");
	
	exit(0);
}

static void RemoveFolder(const char* item) {
	Terminal_ClearLines(2);
	
	printf_info("%s*", item);
	Sys_Delete_Recursive(item);
}

static void ClearDump(void) {
	MemFile mem;
	
	if (Sys_Stat(gProjectConfig)) {
		MemFile_LoadFile_String(&mem, gProjectConfig);
		gVanilla = Config_GetStr(&mem, "z_vanilla");
		MemFile_Free(&mem);
	}
	
	printf_toolinfo(gToolName, "Clearing Project\n");
	
	RemoveFolder(xFmt("rom/actor/%s/", gVanilla));
	RemoveFolder(xFmt("rom/effect/%s/", gVanilla));
	RemoveFolder(xFmt("rom/object/%s/", gVanilla));
	RemoveFolder(xFmt("rom/scene/%s/", gVanilla));
	RemoveFolder(xFmt("rom/sound/sample/%s/", gVanilla));
	RemoveFolder(xFmt("rom/sound/sequence/%s/", gVanilla));
	RemoveFolder(xFmt("rom/sound/soundfont/%s/", gVanilla));
	RemoveFolder(xFmt("rom/system/kaleido/%s/", gVanilla));
	RemoveFolder(xFmt("rom/system/skybox/%s/", gVanilla));
	RemoveFolder(xFmt("rom/system/state/%s/", gVanilla));
	RemoveFolder(xFmt("rom/system/static/%s/", gVanilla));
	
	RemoveFolder(xFmt("rom/lib_code/"));
	RemoveFolder(xFmt("rom/lib_user/"));
	Sys_Delete(gProjectConfig);
	
	gVanilla = StrDup(".vanilla");
	qFree(gVanilla);
}

static void ClearCache(void) {
	printf_toolinfo(gToolName, "Clearing Cache\n");
	
	RemoveFolder("rom/yaz-cache/");
}

static void GZInject() {
	char buffer[512];
	MemFile conf;
	char* path;
	char* wad;
	char* rom;
	char* target;
	
	if (MemFile_LoadFile_String(&conf, gProjectConfig))
		printf_error("Could not open [%s]", gProjectConfig);
	
	wad = Config_GetStr(&conf, "vc_basewad");
	rom = Config_GetStr(&conf, "z_buildrom");
	
	if (!wad || !strcmp(wad, "NULL"))
		printf_error("" PRNT_YELW "vc_basewad" PRNT_RSET " not defined in [%s]]", gProjectConfig);
	
	if (PathIsRel(wad)) wad = PathAbs(wad);
	
	asprintf(&target, "%s.wad", PathAbs(rom));
	asprintf(&rom, "%s%s", PathAbs(rom), sRomType[gBuildTarget]);
	
	path = Config_GetStr(&conf, "vc_dolphin");
	if (path && strcmp(path, "NULL")) {
		if (!StrEnd(path, "/"))
			path = xFmt("%s/", path);
		if (Sys_Stat(path)) {
			Sys_Delete_Recursive(xFmt("%sWii/title/00010001/", path));
			Sys_Delete_Recursive(xFmt("%sWii/title/00000001/", path));
		}
	}
	
	Sys_SetWorkDir(xFmt("%stools/", Sys_AppDir()));
	
	sprintf(
		buffer,
		"gzinject"
#ifdef _WIN32
		".exe"
#endif
		" "
		"-a inject "
		"-w \"%s\" "
		"-m \"%s\" "
		"-p patches/NACE.gzi "
		"-p patches/gz_raphnet_remap.gzi "
		"-p patches/analog_substick.NACE.gzi "
		"-o \"%s\" "
		"--verbose",
		wad,
		rom,
		target
	);
	
	SysExe(buffer);
	Free(target);
	Free(rom);
}

static void ReadProject(Rom* rom, char** input) {
	MemFile* config = &rom->config;
	char* buildRom;
	
	gBaserom = Config_GetStr(config, "z_baserom");
	buildRom = Config_GetStr(config, "z_buildrom");
	gVanilla = StrDup(Config_GetStr(config, "z_vanilla")); qFree(gVanilla);
	
	if (strlen(buildRom) > 128 - strlen(sRomType[1]))
		printf_error("z_buildrom name is too long");
	
	sprintf(gBuildrom[0], "%s%s", buildRom, sRomType[0]);
	sprintf(gBuildrom[1], "%s%s", buildRom, sRomType[1]);
	
	if (*input) {
		if (PathIsAbs(*input)) {
			*input = PathRel(*input);
		}
		
		if (!Sys_Stat(*input))
			printf_error("Provided rom does not exist... [%s]", PathAbs(*input));
		
		foreach(i, sRomType) {
			if (!strcmp(*input, gBuildrom[i])) {
				gBuildTarget = i;
				gCompressFlag = true;
				gDumpFlag = false;
			}
		}
		
		if (gCompressFlag == false) {
			if (!strcmp(gBaserom, "__ROM_NAME__") || !Sys_Stat(gProjectConfig)) {
				printf_toolinfo(gToolName, "Dumping Rom");
				gBaserom = Filename(*input);
				gDumpFlag = true;
				
				Config_ReplaceVariable(config, "z_baserom", gBaserom);
			} else {
				printf_toolinfo(gToolName, "");
				printf_warning("Dump rom [%s] ? " PRNT_DGRY "[y/n]", PathAbs(*input));
				
				if (gDumpRom == true || Terminal_YesOrNo() == true) {
					gBaserom = Filename(*input);
					gDumpFlag = true;
					
					Config_ReplaceVariable(config, "z_baserom", Filename(*input));
				}
				
				Terminal_ClearLines(2);
			}
		}
	}
	
	gBaserom = StrDup(Config_GetStr(config, "z_baserom")); qFree(gBaserom);
	
	if (gDumpFlag && strcmp(*input, gBaserom)) {
		printf_info("Copying Rom to App Path");
		if (Sys_Copy(*input, gBaserom))
			printf_error("Could not copy [%s] to [%s]", PathAbs(*input), PathAbs(gBaserom));
		
		Sys_Sleep(0.1);
		Terminal_ClearLines(2);
	}
	
	if (strcmp(gBaserom, "__ROM_NAME__")) {
		if (!Sys_Stat(gBaserom))
			printf_error("Could not locate your baserom [%s]", PathAbs(gBaserom));
		else
			*input = (char*)gBaserom;
	}
}

static void WriteProjectConfig(MemFile* config, const char* romName, const char* build, const char* vanilla, const char* vcBase, const char* vcPath) {
	Log("Writing [%s]", gProjectConfig);
	MemFile_Reset(config);
	MemFile_Alloc(config, MbToBin(2.5));
	
	Config_WriteComment(config, "Project Settings");
	Config_WriteStr(config, "z_baserom", romName, QUOTES, NO_COMMENT);
	
	Config_WriteStr(config, "z_buildrom", build, QUOTES, "Name used for the rom that is built by z64rom.");
	Config_WriteStr(config, "z_vanilla", vanilla, QUOTES, "Name of the vanilla item folders");
	
	Config_Print(config, "\n");
	Config_WriteComment(config, "Wii VC");
	Config_WriteStr(config, "vc_basewad", vcBase, QUOTES, NULL);
	Config_WriteStr(config, "vc_dolphin", vcPath, QUOTES, "Path to documents folder, not the app folder.");
	
	Config_Print(config, "\n");
	Config_WriteComment(config, "Mips64 Flag");
	
	Config_WriteStr(
		config,
		"gcc_base_flags",
		"-c -G 0 -O1 -std=gnu99 -march=vr4300 -mabi=32 -mips3"
		" "
		"-mno-explicit-relocs -mno-memcpy -mno-check-zero-division"
		" "
		"-fno-common"
		" "
		"-Wall -Wno-builtin-declaration-mismatch"
		" "
		"-Isrc/lib_user -Iinclude/z64hdr -Iinclude/z64hdr/include",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"gcc_actor_flags",
		"",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"gcc_code_flags",
		"-mno-gpopt -fomit-frame-pointer",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"gcc_kaleido_flags",
		"",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"gcc_state_flags",
		"",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"ld_base_flags",
		"-Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"ld_code_flags",
		"-T z64hdr.ld "
		"-T z_lib_user.ld "
		"-T z_object_user.ld "
		"--emit-relocs",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"ld_scene_flags",
		"-T z64hdr_actor.ld "
		"--emit-relocs",
		QUOTES,
		NO_COMMENT
	);
	
	Config_WriteStr(
		config,
		"ld_ulib_flags",
		"-T ulib_linker.ld "
		"-T z_object_user.ld "
		"--emit-relocs",
		QUOTES,
		NO_COMMENT
	);
}

static void WriteProject(Rom* rom, char** input) {
	MemFile* config = &rom->config;
	const char* romName;
	
	Log("Writing [%s]", gProjectConfig);
	MemFile_Reset(config);
	MemFile_Alloc(&rom->config, MbToBin(4.0));
	
	Config_WriteComment(config, "Project Settings");
	if (*input)
		romName = Filename(*input);
	else
		romName = "__ROM_NAME__";
	
	WriteProjectConfig(&rom->config, romName, "build", gVanilla, "NULL", "NULL");
}

static void ReconfigProject(Rom* rom) {
	MemFile mainConfig;
	
	MemFile_LoadFile_String(&mainConfig, "z64project.cfg");
	
	WriteProjectConfig(
		&rom->config,
		Config_GetStr(&mainConfig, "z_baserom"),
		Config_GetStr(&mainConfig, "z_buildrom"),
		Config_GetStr(&mainConfig, "z_vanilla"),
		Config_GetStr(&mainConfig, "vc_basewad"),
		Config_GetStr(&mainConfig, "vc_dolphin")
	);
	
	MemFile_SaveFile_String(&rom->config, "z64project.cfg");
	printf_info("Reconfig OK");
	exit(0);
}

static void ProcessConfig(char** input, Rom* rom) {
	MemFile* config = &rom->config;
	
	rom->config = MemFile_Initialize();
	
	if (!Sys_Stat(gProjectConfig))
		WriteProject(rom, input);
	
	else {
		Log("Reading [%s]", gProjectConfig);
		MemFile_LoadFile_String(config, gProjectConfig);
		MemFile_Realloc(config, config->size * 8);
	}
	
	ReadProject(rom, input);
	
	if (gDumpFlag == true) {
		*input = StrDup(Filename(*input)); qFree(*input);
	} else {
		if (gBuildTarget == ROM_RELEASE) {
			if (Sys_Stat(gBuildrom[ROM_DEV]) > Sys_Stat(gBuildrom[ROM_RELEASE])) {
				MakeClean();
				gMakeForce = true;
				
				return;
			}
		} else {
			if (Sys_Stat(gBuildrom[ROM_RELEASE]) > Sys_Stat(gBuildrom[ROM_DEV])) {
				MakeClean();
				gMakeForce = true;
				
				return;
			}
		}
	}
}

static s32 ProcessArguments(Rom* rom, char* input, char* argv[]) {
	u32 parArg = 0;
	
	if (Arg("log")) Log_NoOutput();
	if (Arg("fun-text")) gTextFlag = true;
	if (Arg("threads")) {
		gThreadNum = Value_Int(argv[parArg]);
		
		if (gThreadNum == 0 || gThreadNum > 512) {
			printf_warning("Dangerous threadNum %d, using default 128 instead.", gThreadNum);
			gThreadNum = 128;
		}
	}
	
	if (Arg("new-actor")) {
		Template_NewActor(argv[parArg], argv[parArg + 1], argv[parArg + 2]);
		
		return 1;
	}
	
	if (Arg("sym")) {
		Sym(argv[parArg]);
		
		return 1;
	}
	
	if (Arg("migrate")) {
		char* mode = argv[parArg];
		Migrate(mode, argv[parArg + 1]);
		
		return 1;
	}
	
	if (Arg("help")) PrintHelp();
	
	if (Arg("pack")) {
		Package_Pack();
		exit(0);
	}
	
	if (Arg("reconfig")) ReconfigProject(rom);
	
	if (Arg("reinstall")) {
		MemFile mem = MemFile_Initialize();
		
		MemFile_Alloc(&mem, 0x10);
		MemFile_SaveFile(&mem, "tools/.installing");
		MemFile_Free(&mem);
	}
	
	if (Arg("clear-dump") || Arg("clear-cache") || Arg("clear-project") || Arg("clear-all")) {
		if (Arg("clear-dump") || Arg("clear-all"))
			ClearDump();
		if (Arg("clear-project") || Arg("clear-all")) {
			RemoveFolder("rom/");
			RemoveFolder("include/object/");
		}
		if (Arg("clear-cache") || Arg("clear-all"))
			ClearCache();
		
		Terminal_ClearLines(2);
	}
	
	if (Arg("clean-samples"))
		Audio_DeleteUnreferencedSamples();
	if (Arg("no-beta"))
		Rom_DeleteUnusedContent();
	
	if (Arg("dump-rom")) gDumpRom = Value_Bool(argv[parArg]);
	if (Arg("dump-audio")) gDumpAudio = Value_Bool(argv[parArg]);
	if (Arg("auto-install")) {
		gAutoInstall = Value_Bool(argv[parArg]);
		
		if (gAutoInstall == false) {
			if (Arg("file-z64hdr"))
				gFile_z64hdr = argv[parArg];
			if (Arg("file-mips64"))
				gFile_mips64 = argv[parArg];
			
			if (Sys_Stat(gFile_mips64) == 0)
				printf_error("Could not stat [%s]", gFile_mips64);
			if (Sys_Stat(gFile_z64hdr) == 0)
				printf_error("Could not stat [%s]", gFile_mips64);
		}
	}
	
	if (Arg("vanilla")) {
		gVanilla = qFree(StrDup(StrUnq(argv[parArg])));
		
		if (isalnum(*gVanilla))
			printf_error("Vanilla folder name should not have alpha-numeric value as the first character!");
	}
	if (Arg("target")) gMakeTarget = argv[parArg];
	if (Arg("info")) gPrintInfo = true;
	if (Arg("yaz")) gCompressFlag = true;
	if (Arg("release")) gBuildTarget = ROM_RELEASE;
	if (Arg("force")) gMakeForce = true;
	if (Arg("clean")) {
		MakeClean();
		
		return 1;
	}
	if (Arg("no-threading")) gThreading = false;
	
	if (Arg("audio-only")) {
		gAudioOnly = true;
		
		if (Arg("dump")) {
			gDumpFlag = true;
			Log("Dump Rom [%s]", argv[parArg]);
			Rom_New(rom, argv[parArg]);
			AudioOnly_Dump(rom);
		} else if (Arg("build")) {
			rom->mem.sampleTbl = MemFile_Initialize();
			rom->mem.fontTbl = MemFile_Initialize();
			rom->mem.seqTbl = MemFile_Initialize();
			rom->mem.seqFontTbl = MemFile_Initialize();
			MemFile_Alloc(&rom->mem.sampleTbl, MbToBin(0.1));
			MemFile_Alloc(&rom->mem.fontTbl, MbToBin(0.1));
			MemFile_Alloc(&rom->mem.seqTbl, MbToBin(0.1));
			MemFile_Alloc(&rom->mem.seqFontTbl, MbToBin(0.1));
			
			gMakeTarget = "sound";
			
			if (!Arg("no-make")) {
				Make_Sound();
				Make_Sequence();
			}
			AudioOnly_Build(rom);
		}
		
		printf_info("OK");
		
		return 1;
	}
	
	if (Arg("inject-vc")) {
		GZInject();
		
		return 1;
	}
	
	if (Arg("build-vc"))
		gCompressFlag = true;
	
	return 0;
}

s32 Main(s32 argc, char* argv[]) {
	char* input = NULL;
	Rom* rom;
	u32 parArg = 0;
	u32 romCount = 0;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	gWorkDir = StrDup(Sys_WorkDir()); qFree(gWorkDir);
	Sys_SetWorkDir(Sys_AppDir());
	
	Calloc(rom, sizeof(struct Rom));
	
	for (s32 i = 1; i < argc; i++) {
		if (StrEndCase(StrUnq(argv[i]), ".z64")) {
			if (romCount > 0)
				printf_error("Too many roms provided as arguments!");
			input = argv[i];
			romCount++;
			Log("Rom [%s]", input);
		}
		
		if (StrStr(argv[i], gProjectConfig)) {
			printf_toolinfo(gToolName, "Release Build");
			gBuildTarget = ROM_RELEASE;
		}
		
		if (StrEndCase(StrUnq(argv[i]), ".mid"))
			Template_NewSequence(argv[i]);
		
		if (StrEndCase(StrUnq(argv[i]), ".wav"))
			Template_NewSample(argv[i]);
		if (StrEndCase(StrUnq(argv[i]), ".aiff"))
			Template_NewSample(argv[i]);
		if (StrEndCase(StrUnq(argv[i]), ".mp3"))
			Template_NewSample(argv[i]);
	}
	
	if (ProcessArguments(rom, input, argv)) goto free;
	ProcessConfig(&input, rom);
	
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
						Sys_Copy(argv[i], filename);
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
				MemFile_Alloc(&umem, 512);
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
	
	if (input == NULL && gDumpRom != false) {
		ItemList list = ItemList_Initialize();
		
		ItemList_List(&list, "", 0, LIST_FILES);
		
		for (s32 i = 0; i < list.num; i++) {
			if (StrEndCase(list.item[i], ".z64") && !StrStr(list.item[i], "build")) {
				printf_toolinfo(gToolName, "");
				printf_info("Looks like you have a rom called " PRNT_REDD "%s " PRNT_RSET "in the same directory.", list.item[i]);
				
				printf_info("Want to use it as your baserom and dump it now? " PRNT_DGRY "[y/n]");
				
				if (Terminal_YesOrNo()) {
					input = StrDup(list.item[i]); qFree(input);
					gDumpFlag = true;
					
					StrRep(rom->config.str, "__ROM_NAME__", input);
					rom->config.size = strlen(rom->config.str);
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
			char* smpVanFldr = xFmt("rom/sound/sample/%s/", gVanilla);
			
			if (gDumpAudio == -1) {
				s32 soundsDumped = false;
				
				if (Sys_Stat(smpVanFldr)) {
					ItemList list = ItemList_Initialize();
					
					ItemList_List(&list, smpVanFldr, -1, LIST_FILES | LIST_RELATIVE);
					
					for (s32 i = 0, j = 0; i < list.num; i++) {
						if (StrEndCase(list.item[i], ".wav")) {
							j++;
							if (j > 400) {
								printf_info("WAV dump already exists.");
								printf_info("Want to redump WAV anyway? " PRNT_DGRY "[y/n]");
								if (Terminal_YesOrNo()) {
									gDumpAudio = true;
								} else {
									gDumpAudio = false;
								}
								Terminal_ClearLines(3);
								
								soundsDumped = true;
								
								break;
							}
						}
					}
					
					ItemList_Free(&list);
				}
				
				if (soundsDumped == false) {
					printf_info("Extract " PRNT_REDD "wav audio samples" PRNT_RSET "?");
					printf_info("This is required if you want to make changes to audio. " PRNT_DGRY "[y/n]");
					if (!Terminal_YesOrNo()) gDumpAudio = false;
					
					printf("\n");
				}
			}
			
			Rom_New(rom, input);
			
			Rom_Dump(rom);
			
			Sys_MakeDir("src/actor/");
			Sys_MakeDir("src/object/");
			Sys_MakeDir("src/scene/");
			Sys_MakeDir("src/effect/");
			Sys_MakeDir("src/sound/sample/");
			Sys_MakeDir("src/sound/sequence/");
			Sys_MakeDir("src/sound/soundfont/");
		} else {
			for (s32 i = 0; i < argc; i++) {
				if (StrStrCase(argv[i], ".zip")) {
					printf_toolinfo(gToolName, "Installing Package(s)");
					printf_info_align("Package", "" PRNT_BLUE "%s", argv[i]);
					Package_Load(argv[i]);
				}
			}
			if (Arg("update")) {
				printf_toolinfo(gToolName, "Updating z64hdr...");
				Tools_Install_z64hdr(true);
				
				goto free;
			}
			
			if (gCompressFlag && gThreading) Rom_Compress();
			if (!Arg("no-make") || gMakeForce) {
				printf_toolinfo(gToolName, "");
				Make(rom, true);
			}
			if (Arg("make-only")) goto free;
			
			Rom_New(rom, input);
			Rom_Build(rom);
		}
		printf_info("Done!\n");
	}
	
	printf_toolinfo(gToolName, "Nothing provided, nothing happens!");
	
	if (input)
		MemFile_SaveFile_String(&rom->config, "z64project.cfg");
	
	if (Arg("build-vc")) {
		if (rom->file.size <= 0x2015000)
			GZInject();
		else
			printf_warning("Compressed rom is too big to be injected to a wad. Consider running z64rom with --no-beta to remove unused beta content.");
	}
	
#ifdef _WIN32
	if (argc == 1) {
		printf_getchar("Press enter to exit.");
	}
#endif
	
free:
	
	FileSys_Free();
	Rom_Free(rom);
	Free(rom);
	Sound_Xm_Stop();
	Log_Free();
	
	return 0;
}
