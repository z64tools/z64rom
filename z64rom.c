#include "src/z64rom.h"
#include "src/Make.h"
#include "src/Package.h"
#include <xm.h>

const char* gToolName = PRNT_BLUE "z64rom " PRNT_GRAY "0.8.7";

s32 gDumpRom = -1;
s32 gDumpAudio = -1;
s32 gAutoInstall = -1;
const char* gFile_z64hdr;
const char* gFile_mips64;

u32 gCompressFlag = false;
s32 gPrintInfo;
s32 gMakeForce;
const char* gMakeTarget;
u32 gThreading = true;
s32 gDumpFlag;

s32 gAudioOnly;
s32 gBuildTarget = ROM_DEV;
char gBuildrom[2][128] = {
	/* "build-release.z64", */
	/* "build-dev.z64", */
};

char* gVanilla = ".vanilla";
const char* gProjectConfig = "z64project.toml";
const char* gBaserom = NULL;

const char* sRomType[] = { "-release.z64", "-dev.z64" };
const char* sHelp[][2] = {
	{ "!" PRNT_BLUE "MANAGING", },
	{ "update",                        "update z64hdr" },
	{ "reinstall",                     "reinstall z64hdr & mips64-binutils" },
	{ "clear-project",                 NULL },
	{ "clear-cache",                   NULL },
	{ "clean",                         "clean build files" },
	{ "clean-samples",                 "clean unreferenced samples" },
	{ NULL,                            NULL },
	
	{ "!" PRNT_BLUE "NO PROMPT", },
	{ "!" PRNT_DGRY "Do not prompt questions when doing these operations.", },
	{ "dump-rom     [bool]",           "" },
	{ "dump-audio   [bool]",           "" },
	{ "auto-install [bool]",           "" },
	{ "!" PRNT_DGRY "If auto-install is \'false\', provide file-x arguments", },
	{ "file-z64hdr  [file]",           "" },
	{ "file-mips64  [file]",           "" },
	{ NULL,                            NULL },
	
	{ "!" PRNT_BLUE "STRING OPTION", },
	{ "vanilla [str]",                 "vanilla folder name. default: .vanilla" },
	{ "target  [str]",                 "make target: \"audio\" / \"code\"" },
	{ NULL,                            NULL },
	
	{ "!" PRNT_BLUE "VARIOUS", },
	{ "info",                          "print slots" },
	{ "yaz",                           "compress" },
	{ "release",                       "build release" },
	{ "log",                           "print logs on close" },
	{ "force",                         "force builds" },
	{ "make-only",                     "no build, only make" },
	{ NULL,                            NULL },
	
	{ "!" PRNT_BLUE "NO", },
	{ "no-wait",                       NULL },
	{ "no-make",                       NULL },
	{ "no-beta",                       NULL },
	{ NULL,                            NULL },
	
	{ "!" PRNT_BLUE "RENAME", },
	{ "zmap",                          "rename all rooms to .zmap" },
	{ "zroom",                         "rename all rooms to .zroom" },
	{ NULL,                            NULL },
	
	{ "!" PRNT_BLUE "AUDIO ONLY", },
	{ "audio-only",                    NULL },
	{ "dump",                          NULL },
	{ "build",                         NULL },
	
	{ "!" PRNT_BLUE "INFO", },
	{ "actor [id]",                    "" },
	{ "dma   [id]",                    "" },
	{ "scene [id]",                    "" },
	{ NULL,                            NULL },
};

static void Main_PrintHelp(void) {
	printf_toolinfo(gToolName, "Help");
	foreach(i, sHelp) {
		if (sHelp[i][0]) {
			if (sHelp[i][0][0] == '!')
				printf("" PRNT_RSET "%s", sHelp[i][0] + 1);
			
			else {
				printf("" PRNT_RSET " --%-15s ", sHelp[i][0]);
				if (sHelp[i][1])
					printf("" PRNT_DGRY "%s", sHelp[i][1]);
			}
		}
		printf("\n");
	}
	
	exit(0);
}

static void RemoveFolder(const char* item) {
	Terminal_ClearLines(2);
	
	printf_info("%s*", item);
	Sys_Delete_Recursive(item);
}

static void Main_ClearProject(void) {
	MemFile mem;
	
	if (Sys_Stat(gProjectConfig)) {
		MemFile_LoadFile_String(&mem, gProjectConfig);
		gVanilla = Toml_GetStr(&mem, "z_vanilla");
		MemFile_Free(&mem);
	}
	
	printf_toolinfo(gToolName, "Clearing Project\n");
	
	RemoveFolder(HeapPrint("rom/actor/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/effect/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/object/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/scene/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/sound/sample/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/sound/sequence/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/sound/soundfont/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/system/kaleido/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/system/skybox/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/system/state/%s/", gVanilla));
	RemoveFolder(HeapPrint("rom/system/static/%s/", gVanilla));
	Sys_Delete(gProjectConfig);
	
	gVanilla = StrDup(".vanilla");
	qFree(gVanilla);
}

static void Main_ClearCache(void) {
	printf_toolinfo(gToolName, "Clearing Cache\n");
	
	RemoveFolder("rom/yaz-cache/");
}

static void Main_WiiVC() {
	char buffer[512];
	MemFile conf;
	char* path;
	char* wad;
	char* rom;
	char* target;
	
	if (MemFile_LoadFile_String(&conf, gProjectConfig))
		printf_error("Could not open [%s]", gProjectConfig);
	
	path = Toml_GetStr(&conf, "vc_dolphin");
	wad = Toml_GetStr(&conf, "vc_basewad");
	rom = Toml_GetStr(&conf, "z_buildrom");
	
	if (!strcmp(path, "NULL"))
		printf_error("" PRNT_YELW "vc_dolphin" PRNT_RSET " not defined in [%s]]", gProjectConfig);
	if (!strcmp(wad, "NULL"))
		printf_error("" PRNT_YELW "vc_basewad" PRNT_RSET " not defined in [%s]]", gProjectConfig);
	
	if (!StrEnd(path, "/"))
		path = HeapPrint("%s/", path);
	if (PathIsRel(wad))
		wad = PathAbs(wad);
	
	asprintf(&target, "%s.wad", PathAbs(rom));
	asprintf(&rom, "%s%s", PathAbs(rom), sRomType[gBuildTarget]);
	Sys_Delete_Recursive(HeapPrint("%sWii/title/00010001/", path));
	Sys_Delete_Recursive(HeapPrint("%sWii/title/00000001/", path));
	
	Sys_SetWorkDir(HeapPrint("%stools/", Sys_AppDir()));
	
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
		StrRep(tmp, from, to);
		
		if (Sys_Rename(list.item[i], tmp)) {
			printf_warning("Could not rename [%s] to \"%s\" format", list.item[i], to);
		}
		times++;
	}
	
	ItemList_Free(&list);
	
	printf_info("%d rooms renamed to *%s.", times, to);
}

static s32 Main_PreArgs(Rom* rom, char* input, char* argv[]) {
	u32 parArg = 0;
	
	if (Arg("log")) Log_NoOutput();
	
	if (Arg("reinstall")) {
		MemFile mem = MemFile_Initialize();
		
		MemFile_Malloc(&mem, 0x10);
		MemFile_SaveFile(&mem, "tools/.installing");
		MemFile_Free(&mem);
	}
	
	if (Arg("clear-project") || Arg("clear-cache") || Arg("clear-all")) {
		if (Arg("clear-project") || Arg("clear-all")) {
			Main_ClearProject();
		}
		
		if (Arg("clear-cache") || Arg("clear-all")) {
			Main_ClearCache();
		}
		
		Terminal_ClearLines(2);
	}
	
	if (Arg("clean-samples")) {
		Audio_DeleteUnreferencedSamples();
		
		return 1;
	}
	
	if (Arg("no-beta")) {
		Rom_DeleteUnusedContent();
		
		return 1;
	}
	
	if (Arg("zmap")) {
		Main_RenameRooms(".zroom", ".zmap");
		
		return 1;
	}
	if (Arg("zroom")) {
		Main_RenameRooms(".zmap", ".zroom");
		
		return 1;
	}
	
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
	
	if (Arg("vanilla")) gVanilla = argv[parArg];
	if (Arg("target")) gMakeTarget = argv[parArg];
	if (Arg("info")) gPrintInfo = true;
	if (Arg("yaz")) gCompressFlag = true;
	if (Arg("release")) gBuildTarget = ROM_RELEASE;
	if (Arg("force")) gMakeForce = true;
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
	if (Arg("no-threading")) gThreading = false;
	
	if (Arg("actor") && input) {
		u32 id = Value_Int(argv[parArg]);
		
		Rom_New(rom, input);
		Rom_Debug_ActorEntry(rom, id);
		
		return 1;
	}
	if (Arg("dma") && input) {
		u32 id = Value_Int(argv[parArg]);
		
		Rom_New(rom, input);
		Rom_Debug_DmaEntry(rom, id);
		
		return 1;
	}
	if (Arg("scene") && input) {
		u32 id = Value_Int(argv[parArg]);
		
		Rom_New(rom, input);
		Rom_Debug_SceneEntry(rom, id);
		
		return 1;
	}
	
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
		Main_WiiVC();
		
		return 1;
	}
	
	if (Arg("build-vc"))
		gCompressFlag = true;
	
	return 0;
}

static void Main_ReadProject(Rom* rom, char** input) {
	MemFile* config = &rom->config;
	char* buildRom;
	
	gBaserom = Toml_GetStr(config, "z_baserom");
	buildRom = Toml_GetStr(config, "z_buildrom");
	gVanilla = StrDup(Toml_GetStr(config, "z_vanilla")); qFree(gVanilla);
	
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
				
				Toml_ReplaceVariable(config, "z_baserom", gBaserom);
			} else {
				printf_toolinfo(gToolName, "");
				printf_warning("Dump rom [%s] ? " PRNT_DGRY "[y/n]", PathAbs(*input));
				
				if (gDumpRom == true || Terminal_YesOrNo() == true) {
					gBaserom = Filename(*input);
					gDumpFlag = true;
					
					Toml_ReplaceVariable(config, "z_baserom", Filename(*input));
				}
				
				Terminal_ClearLines(2);
			}
		}
	}
	
	gBaserom = StrDup(Toml_GetStr(config, "z_baserom")); qFree(gBaserom);
	
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

static void Main_WriteProject(Rom* rom, char** input) {
	MemFile* config = &rom->config;
	
	Log("Writing [%s]", gProjectConfig);
	MemFile_Reset(config);
	MemFile_Malloc(config, MbToBin(2.5));
	
	Toml_WriteComment(config, "Project Settings");
	if (*input)
		Toml_WriteStr(config, "z_baserom", Filename(*input), QUOTES, NO_COMMENT);
	else
		Toml_WriteStr(config, "z_baserom", "__ROM_NAME__", QUOTES, NO_COMMENT);
	
	Toml_WriteStr(config, "z_buildrom", "build", QUOTES, "Name used for the rom that is built by z64rom.");
	Toml_WriteStr(config, "z_vanilla", gVanilla, QUOTES, "Name of the vanilla item folders");
	
	Toml_Print(config, "\n");
	Toml_WriteComment(config, "Wii VC");
	Toml_WriteStr(config, "vc_basewad", "NULL", QUOTES, NULL);
	Toml_WriteStr(config, "vc_dolphin", "NULL", QUOTES, "Path to documents folder, not the app folder.");
	
	Toml_Print(config, "\n");
	Toml_WriteComment(config, "Mips64 Flag");
	
	Toml_WriteStr(
		config,
		"mips64_gcc_flags",
		"-c -Iinclude/z64hdr -Iinclude/z64hdr/include "
		"-Isrc/lib_user -G 0 -O1 -fno-reorder-blocks -fno-common -std=gnu99 -march=vr4300 -mabi=32"
		" -mips3 -mno-explicit-relocs -mno-memcpy -mno-check-zero-division -Wall"
		" -Wno-builtin-declaration-mismatch",
		QUOTES,
		NO_COMMENT
	);
	Toml_WriteStr(
		config,
		"mips64_gcc_flags_code",
		"-mno-gpopt -fomit-frame-pointer",
		QUOTES,
		NO_COMMENT
	);
	Toml_WriteStr(
		config,
		"mips64_ld_flags",
		"-Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ "
		"-T z64hdr.ld "
		"-T objects.ld "
		"-T z_lib_user.ld "
		"--emit-relocs",
		QUOTES,
		NO_COMMENT
	);
	Toml_WriteStr(
		config,
		"ulib_ld_flags",
		"-Lrom/lib_user -Linclude/z64hdr/oot_mq_debug/ -Linclude/z64hdr/common/ -Linclude/ "
		"-T ulib_linker.ld "
		"-T objects.ld "
		"--emit-relocs",
		QUOTES,
		NO_COMMENT
	);
}

static void Main_Config(char** input, Rom* rom) {
	MemFile* config = &rom->config;
	
	rom->config = MemFile_Initialize();
	
	if (!Sys_Stat(gProjectConfig))
		Main_WriteProject(rom, input);
	
	else {
		Log("Reading [%s]", gProjectConfig);
		MemFile_LoadFile_String(config, gProjectConfig);
	}
	
	Main_ReadProject(rom, input);
	
	if (gDumpFlag == true) {
		*input = StrDup(Filename(*input)); qFree(*input);
	} else {
		if (gBuildTarget == ROM_RELEASE) {
			if (Sys_Stat(gBuildrom[ROM_DEV]) > Sys_Stat(gBuildrom[ROM_RELEASE])) {
				gMakeForce = true;
				
				return;
			}
		} else {
			if (Sys_Stat(gBuildrom[ROM_RELEASE]) > Sys_Stat(gBuildrom[ROM_DEV])) {
				gMakeForce = true;
				
				return;
			}
		}
	}
}

s32 Main(s32 argc, char* argv[]) {
	char* input = NULL;
	Rom* rom;
	u32 parArg = 0;
	u32 romCount = 0;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Sys_SetWorkDir(Sys_AppDir());
	
	if (Arg("help")) Main_PrintHelp();
	
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
	}
	
	if (Main_PreArgs(rom, input, argv)) goto free;
	Main_Config(&input, rom);
	
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
					input = StrDup(list.item[i]); qFree(input);
					gDumpFlag = true;
					
					StrRep(rom->config.str, "__ROM_NAME__", input);
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
			char* smpVanFldr = HeapPrint("rom/sound/sample/%s/", gVanilla);
			
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
			
			if (!Arg("no-make") || gMakeForce) {
				printf_toolinfo(gToolName, "");
				Make(rom, true);
			}
			if (Arg("make-only")) goto free;
			if (gCompressFlag && gThreading) Rom_Compress();
			
			Rom_New(rom, input);
			Rom_Build(rom);
		}
		printf_info("Done!\n");
	}
	
	printf_toolinfo(gToolName, "Nothing provided, nothing happens!");
	
	MemFile_SaveFile_String(&rom->config, "z64project.toml");
	
	if (Arg("build-vc")) {
		if (rom->file.dataSize <= 0x2015000)
			Main_WiiVC();
		else
			printf_warning("Compressed rom is too big to be injected to a wad. Consider running z64rom with --no-beta to remove unused beta content.");
	}
	
#ifdef _WIN32
	if (!Arg("no-wait")) {
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
